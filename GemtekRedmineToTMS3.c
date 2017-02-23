//GemtekRedmineToTMS2.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "cJSON.h"
#include "xlsxwriter.h"
#include <unistd.h>
struct string
{
    char *ptr;
    size_t len;
};
////////////////////////
struct data
{
    int issue_id;
    char *tracker_name;
    char *status_name;
    char *priority_name;
    char *issue_subject;
    char *issue_updated_on;
    char *project_name;
    // excel content
    int issue_spent_hours;
    char *job_content;
    char *date_sheet;
    char *date_day;
    char *BU_content;
};

void parse_array(cJSON *array,struct data *list)
{
    cJSON *issue = array ? array->child : 0;
    int count=0;
    while (issue)
    {
        cJSON *project = cJSON_GetObjectItem(issue, "project");
        cJSON *tracker = cJSON_GetObjectItem(issue, "tracker");
        cJSON *status = cJSON_GetObjectItem(issue, "status");
        cJSON *priority = cJSON_GetObjectItem(issue, "priority");

        //no use
        //cJSON *author = cJSON_GetObjectItem(issue, "author");
        //cJSON *assigned_to = cJSON_GetObjectItem(issue, "assigned_to");

        list[count].issue_id=cJSON_GetObjectItem(issue, "id")->valueint;
        list[count].tracker_name=cJSON_GetObjectItem(tracker, "name")->valuestring;
        list[count].status_name=cJSON_GetObjectItem(status, "name")->valuestring;
        list[count].priority_name=cJSON_GetObjectItem(priority, "name")->valuestring;
        list[count].issue_subject=cJSON_GetObjectItem(issue, "subject")->valuestring;
        list[count].issue_updated_on=cJSON_GetObjectItem(issue, "updated_on")->valuestring;
        list[count].project_name=cJSON_GetObjectItem(project, "name")->valuestring;

        count++;
        issue=issue->next;
    }
}

void write_excel(struct data *list)
{
    if(!list[0].job_content)
    {
        printf("Nothing to bulid.\n");
        exit(0);
    }
    // Create a new workbook and add a worksheet.
    char buffer[10]=".xlsx";
    char filename [30]="";
    strcpy(filename,list[0].date_sheet);
    strcat(filename,buffer);
    lxw_workbook  *workbook  = workbook_new(filename);
    lxw_worksheet *worksheet = workbook_add_worksheet(workbook,  list[0].date_sheet);
    // Add a format.
    lxw_format *format = workbook_add_format(workbook);
    // Set the bold property for the format
    format_set_bold(format);
    // Change the column width for clarity.

    worksheet_set_column(worksheet, 0, 2, 35, NULL);
    // Write some simple text.
    worksheet_write_string(worksheet, 0, 0, "Project Name", NULL);
    worksheet_write_string(worksheet, 0, 1, "Task Name", NULL);
    worksheet_write_string(worksheet, 0, 2, "工作項目", NULL);
    worksheet_write_string(worksheet, 0, 3, "BU", NULL);
    worksheet_write_string(worksheet, 0, 4, "日", NULL);
    worksheet_write_string(worksheet, 0, 5, "時數", NULL);

    // Text with formatting.
    int i=0;
    while(list[i].issue_subject)
    {
        worksheet_write_string(worksheet, i+1, 0, "Other", NULL);
        worksheet_write_string(worksheet, i+1, 2, list[i].job_content,NULL);
        worksheet_write_string(worksheet, i+1, 3, list[i].BU_content, NULL);
        worksheet_write_string(worksheet, i+1, 4, list[i].date_day, NULL);
        worksheet_write_number(worksheet, i+1, 5, list[i].issue_spent_hours, NULL);
        i++;
    }
    workbook_close(workbook);
}

void init_string(struct string *s)
{
    s->len = 0;
    s->ptr = malloc(s->len+1);
    if (s->ptr == NULL)
    {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s)
{
    size_t new_len = s->len + size*nmemb;
    s->ptr = realloc(s->ptr, new_len+1);
    if (s->ptr == NULL)
    {
        fprintf(stderr, "realloc() failed\n");
        exit(EXIT_FAILURE);
    }
    memcpy(s->ptr+s->len, ptr, size*nmemb);
    s->ptr[new_len] = '\0';
    s->len = new_len;
    return size*nmemb;
}

char *string_concat(char *str1, char *str2)
{
    // 計算所需的陣列長度
    int length=strlen(str1)+strlen(str2)+1;

    // 產生新的陣列空間
    char *result = (char*)malloc(sizeof(char) * length);

    // 複製第一個字串至新的陣列空間
    strcpy(result, str1);
    // 串接第二個字串至新的陣列空間
    strcat(result, str2);

    return result;
}

char *bracket(char *s)
{
    char *b1 = (char *)malloc(strlen(s)+4);
    char *b2 = "]";
    strcpy(b1, "[");
    strcat(b1, s);
    strcat(b1,b2);
    return b1;
}
void job_content(struct data *list)
{
    if(!list)
    {
        printf("error!\n");
        exit(0);
    }
    int i=0;
    while(list[i].tracker_name)
    {
        // (int)issue_id to (char)
        char buffer [11];
        sprintf(buffer ,"%d",list[i].issue_id);

        list[i].job_content=string_concat(bracket( buffer),bracket (list[i].tracker_name) );
        list[i].job_content=string_concat(list[i].job_content,bracket (list[i].status_name) );
        list[i].job_content=string_concat(list[i].job_content,bracket(list[i].priority_name));
        list[i].job_content=string_concat(list[i].job_content,"-");
        list[i].job_content=string_concat(list[i].job_content,list[i].issue_subject);
        printf("Issue=%s\n",list[i].job_content );
        i++;
    }
}
void  day_content(struct data *list)
{
    if(!list)
    {
        printf("error!\n");
        exit(0);
    }
    int i=0;
    while(list[i].issue_updated_on)
    {
        char *t1[6];
        char *t2[6];
        char *test1 = strtok(list[i].issue_updated_on, "-");
        int j = 0;

        while (test1 != NULL)
        {
            t1[j] = test1;
            //printf("t1[%d]=%s\n",j, t1[j]);
            test1 = strtok(NULL, "-");
            j++;
        }
        char *test2 = strtok(t1[2], "T");
        int k=0;
        while (test2 != NULL)
        {
            t2[k] = test2;
            //printf("t2[%d]=%s\n",k, t2[k]);
            test2 = strtok(NULL, "T");
            k++;
        }
        list[i].date_sheet=string_concat(t1[0],t1[1]);
        list[i].date_day=t2[0];
        //printf("dddd%s\n", list[i].date_sheet);
        i++;
    }
}

void BU_content(struct data *list)
{
    if(!list)
    {
        printf("BU error!\n");
        exit(0);
    }
    char *All_BU="All BU";
    char *BU1="BU1";
    char *BU2="BU2";
    char *BU3="BU3";
    char *FBD="FBD";
    char *GIOT="GIOT";
    char *RDD_Sales="RDD Sales";
    char *SMU="SMU";
    char *Test_project="Test-project";
    int i=0;
    while (list[i].project_name)
    {
        //printf("%s\n",list[i].project_name );
        if(strcmp(list[i].project_name,All_BU)==0 )
        {
            list[i].BU_content=All_BU;
        }
        else if(strcmp(list[i].project_name,BU1)==0 )
        {
            list[i].BU_content=BU1;
        }
        else if(strcmp(list[i].project_name,BU2)==0 )
        {
            list[i].BU_content=BU2;
        }
        else if(strcmp(list[i].project_name,BU3)==0 )
        {
            list[i].BU_content=BU3;
        }
        else if(strcmp(list[i].project_name,FBD)==0 )
        {
            list[i].BU_content=FBD;
        }
        else if(strcmp(list[i].project_name,GIOT)==0 )
        {
            list[i].BU_content=GIOT;
        }
        else if(strcmp(list[i].project_name,RDD_Sales)==0 )
        {
            list[i].BU_content=RDD_Sales;
        }
        else if(strcmp(list[i].project_name,SMU)==0 )
        {
            list[i].BU_content=SMU;
        }
        else if(strcmp(list[i].project_name,Test_project)==0 )
        {
            list[i].BU_content=Test_project;
        }
        else
        {
            list[i].BU_content="Other";
            //printf("Other==%s\n",list[i].BU_content );
        }
        i++;
    }
}
void worktime(struct data *list,int i,char *headtitle)
{
        //struct curl_slist *Dheaders = NULL;
        char Jurl[100]="http://redmine.gemteks.com/issues/";
        char Jtail[100]=".json?include=journals";
        char buffer [11];
        sprintf(buffer ,"%d",list[i].issue_id);
        strcat(Jurl,buffer);
        strcat(Jurl,Jtail);
        //printf("Add%p\n",buffer );
        int try_times=0;
    AGAIN: ;
        struct curl_slist *Dheaders = NULL;
        CURL *hnd;
        hnd = curl_easy_init();
        if(hnd)
        {
            CURLcode rest;
            struct string q;
            init_string(&q);
            //Dheaders = curl_slist_append(Dheaders,"X-Redmine-API-Key:2a6dcde8439f98ee2cfa5fc2b3303178701e1845");
            Dheaders = curl_slist_append(Dheaders,headtitle);
            Dheaders = curl_slist_append(Dheaders, "Content-Type: application/json");
            Dheaders = curl_slist_append(Dheaders, "cache-control: no-cache");

            curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "GET");
            curl_easy_setopt(hnd, CURLOPT_URL, Jurl);
            printf("%s\n",Jurl );
            //printf("%s\n", headtitle);
            curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, Dheaders);
            curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, writefunc);
            curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &q);
            rest=curl_easy_perform(hnd);
            if(rest != CURLE_OK)
            {
                printf("rest=%d ",rest );
                printf("Fetch Error Go to AGAIN\n");
                sleep(1);
                try_times++;
                if(try_times>9)
                {
                    printf("Error %d times. 10 Times Program will Terminate!\n", try_times);
                    exit(0);
                }
                goto AGAIN;
            }
            //printf("%s\n",q.ptr );
            cJSON * root = cJSON_Parse(q.ptr);
            cJSON *issue = cJSON_GetObjectItem(root, "issue");

            list[i].issue_spent_hours = cJSON_GetObjectItem(issue, "spent_hours")->valueint ;
            //printf("AAA%d\n",cJSON_GetObjectItem(issue, "id")->valueint  );
            if(list[i].issue_spent_hours<=0||list[i].issue_spent_hours>24)
            {
                if(list[i].issue_spent_hours<=0)
                {
                    list[i].issue_spent_hours=1;
                    printf("spent_hours must >0 ,default set 1.0\n");
                }
                else if (list[i].issue_spent_hours>24)
                {
                    list[i].issue_spent_hours=24;
                    printf("spent_hours must <24 ,default set 24\n");
                }
            }
            free(q.ptr);
            /* always cleanup */
            curl_slist_free_all(Dheaders);
            curl_easy_cleanup(hnd);
        }
}

int main(int argc, char const *argv[])
{
    struct data list[1000];
    char APIurl[256]="http://redmine.gemteks.com/issues.json?created_on=%3E%3C";
    char day_tail[30]="&limit=200";
    //personal
    char headtitle[256]="X-Redmine-API-Key:";
    printf("Please Enter Your%s\n",headtitle);
    char headkey[256];
    scanf ("%s",headkey);
    strcat(headtitle,headkey);

    char day_range[256];
    printf("Enter Day_range (Example: 2017-02-01|2017-02-22 )\n");
    scanf ("%s",day_range);
    strcat(APIurl,day_range);
    strcat(APIurl,day_tail);
    int main_try_times =0;
    mainAGAIN: ;
    CURL *curl;
    //CURLcode res;
    struct curl_slist *headers = NULL;
    //personal
    curl = curl_easy_init();
    if(curl)
    {
        CURLcode res;
        struct string s;
        init_string(&s);

        //headers = curl_slist_append(headers,"X-Redmine-API-Key:adba60d75b49ca2761e97bec0fb5e7a135cb2567");
        headers = curl_slist_append(headers,headtitle);
        headers = curl_slist_append(headers, "cache-control: no-cache");
        headers = curl_slist_append(headers, "Content-Type: application/json");

        //curl_easy_setopt(curl, CURLOPT_URL, "http://redmine.gemteks.com/issues.json?created_on=%3E%3C2017-02-01|2017-02-22&limit=100");
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
        curl_easy_setopt(curl, CURLOPT_URL,APIurl);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        res=curl_easy_perform(curl);
        if(res != CURLE_OK)
        {
                printf("RES=%d ",res );
                printf("Fetch Error Go to mainAGAIN\n");
                sleep(1);
                main_try_times++;
                if(main_try_times>9)
                {
                    printf("Error %d times.10 Times Program will Terminate!\n", main_try_times);
                    exit(0);
                }
                goto mainAGAIN;
        }
        //printf("%s\n",s.ptr );
        parse_array(cJSON_GetObjectItem(cJSON_Parse(s.ptr),"issues"),list);

        day_content(list);
        job_content(list);
        BU_content(list);

        int i=0;
        while(list[i].issue_id)
        {
            worktime(list,i,headtitle);
            i++;
        }

        //printf("OUT=%.2lf\n",list[0].issue_spent_hours);
        //printf("%s\n",list[1].issue_updated_on );
        //printf("Test==%s\n",list[1].BU_content);

        write_excel(list);
        free(s.ptr);

        /* always cleanup */
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    return 0;
}