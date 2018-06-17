/*  ***	Identify and removes duplicate files from a given directory. ***
 *	
 *	Input: [options] directory_name
 *	Working:
 *		- Parse cmd line args.
 *		- Open and read directory for file names, get their size and file type info.
 *		- Select regular files only and create a sorted Linked list with filename and size as its content.
 * 		- Seperate the nodes with same file size and add them into a new nested linked list. Delete the nodes having a non-repeating size.
 *
 *		- Iterate over the nested linked list and determine which files are identical and store them in a new nested linked list.
 *
 *		- Check the options:
 *			-> if 'i', print the linked list with identical files.
 *			-> if 'r', delete all the duplicates and create a link to original file.
 *
 */


#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<dirent.h>
#include<errno.h>

#include"SLL.h"

extern int errno;
struct file_data
{
	int size;
	char *name;	//name of the file
};


void print_files(void *ptr);
void free_mem(void *ptr);
void free_nested_list(SLL **__repeated);
void print_repeated(SLL *repeated,void (*print_files)(void *ptr));
int compare_size(void * p1, void *p2);
int identical(struct file_data *ptr1, struct file_data *ptr2);
int read_directory(char* dir_name,SLL **files);
int create_repeated(SLL **files, SLL **only_repeated);
int create_identical(SLL **__repeated, SLL **only_identical);


//find if files have same content or not, if yes, return 1.
int identical(struct file_data *ptr1, struct file_data *ptr2)
{
	FILE *fp1, *fp2;
	int exit_status = 0;
	char ch1, ch2;

	fp1 = fopen(ptr1->name,"r");
	if(!fp1){
		printf("%s :\n",ptr1->name);
		perror("fopen");
		exit_status = -1;
		goto EXIT;
	}

	fp2 = fopen(ptr2->name,"r");
	if(!fp2){
		printf("%s :\n",ptr2->name);
		perror("fopen");
		exit_status = -1;
		goto EXIT_1;
	}

	do{
		ch1 = fgetc(fp1);
		ch2 = fgetc(fp2);
	}
	while( ( ch1 == ch2 ) && ( ch1 != EOF )  && ( ch2 != EOF ) );

	if( ( ch1 == ch2 ) )
		exit_status = 1;	//files are Identical
	else
		exit_status = 0;	//Not

	fclose(fp2);
EXIT_1:
	fclose(fp1);
EXIT:
	return exit_status;
}


//This function finds out the identical files in the only_repeated list.
int create_identical(SLL **__repeated, SLL **only_identical)
{
	SLL * prev_node, *cur_node, *old;
	SLL *repeated = *__repeated;
	int count=0,exit_status=0;
	while( repeated ){
		prev_node = (SLL*) repeated->val;

		//Iterate over the repeated[i] list and find identical files
		while( prev_node ){
			old= prev_node;
			cur_node = prev_node->next;
			count=0;
			while( cur_node ){

//				printf("cur_node\t");
//				print_files(cur_node->val);
				if ( identical(prev_node->val,cur_node->val) == 1 ){
	
					if(!count)		
						if( -1 == (int) add_node(only_identical,0,END,NULL) ){	//add_end
							exit_status=-1;
							goto EXIT;
						}
					attach_node(only_identical,(SLL**)&repeated->val,cur_node,old );
					count++;
				/*	printf("\n########\n");
					print_repeated(*__repeated,print_files);
					printf("\n########\n");
			*/	}
				else
					old = cur_node;

				cur_node = old->next;
			}
			if(count)
				attach_node(only_identical,(SLL**)&repeated->val,prev_node,old);
			prev_node = prev_node->next;
		}
		
		repeated = repeated->next;
	}
EXIT:
	return exit_status;
}



int main(int argc, char** argv)
{
	_Bool options[2]={0,0};
	char *dir_name=NULL, link_name[256];
	int exit_status=0, cnt=0, ret=0 ;
	SLL *files=NULL, *only_repeated=NULL, *only_identical=NULL;

	if( argc>5 || argc<3 ){
		printf("USAGE: %s [OPTIONS] [ABSOLUTE DIRECTORY PATH]\n[OPTIONS]\n  \"i\" \tFind identical files based on Size and Content Match.\n  \"r\"\tDelete the identical files and create a softlink to original file.\n",argv[0]);		
		exit(-1);
	}

//////////////Get Cmdline args/////////////////

	//check if necessary options are given to execute.	
	while(cnt < argc){
		if( argv[cnt][0] == '-'  && strchr( argv[cnt],'d' )    ){
			cnt=1;break;

		}
		cnt++;
	}

	if(cnt!=1){
		printf("ERROR: missing Directory path and its options\n");
		exit_status = -1;
		goto EXIT;
	}
	
	//parse options
	while( ( ret = getopt(argc,argv,"ird:")  ) != -1    )
	{
		switch(ret){
			case 'i':
				options[0]=1;
				break;
			case 'r':
				options[1]=1;
				break;
			case 'd':
				dir_name=optarg;
				break;
		}
	}


///////////////////////////////////////////////


	
//Read Directory and generate the linked list for all files with size
	if( -1 == read_directory(dir_name,&files) ){
		exit_status = -1;
		goto EXIT_1;
	}


//Create an linked list with nodes of same size

	if( -1 == create_repeated(&files,&only_repeated) ){
		exit_status = -1;
		goto EXIT_2;
	}

//Create an identical list from only_repeated list.

	if( -1 == create_identical(&only_repeated,&only_identical) ){
		exit_status = -1;
		goto EXIT_3;
	}

//Process the cmd line arguments
	if(options[0]){
		print_repeated(only_identical,print_files);
	}

	if(options[1]){

		//delete the all duplicate files and create a softlink for original file. 			
		SLL *ptr,*repeated = only_identical;
		SLL *node,*ptr1;
		while(repeated){
			node = (SLL*)repeated->val;	//keeping original file
			ptr = node->next;
//			print_files(node->val);
			while( ptr ){
//				print_files(ptr->val);
				ptr1=ptr->next;
				unlink( ( (struct file_data*) ptr->val )->name );
				delete_node(&node,node,NO,FREE,free_mem);
				ptr=ptr1;
			}
			//Creating soft link
		
			memset(link_name,0,sizeof(link_name));	
			strcpy(link_name,( (struct file_data*) node->val )->name );
			strcat(link_name,"_link");
			if ( -1 == symlink( ( (struct file_data*) node->val )->name , link_name ) )
			{
				perror("symlink");
				exit_status=-1;
				goto EXIT_1;
				//error
			}
			repeated=repeated->next;
		}
		

	}//options

EXIT_3:
	free_nested_list(&only_repeated);
EXIT_2:
	free_nested_list(&only_identical);
EXIT_1:
	free_list(&files,FREE,free_mem);	// free the files List.
EXIT:
	return exit_status;
}


//free nested linked list
void free_nested_list(SLL **__repeated)
{
	SLL *ptr = NULL;
	SLL *repeated = *__repeated;
//	printf("repeat %u\n",repeated);
	while(repeated){
		ptr = (SLL*)repeated->val;
	//	printf("In loop ptr %u\n",repeated);
		//free_list(&ptr,FREE,free_mem);
		free_list( (SLL**)&repeated->val, FREE, free_mem);
		repeated=repeated->next;
	//	printf("Inloop repeat %u\n",repeated);
	}
	free_list(__repeated,NOT_FREE,NULL);

}




//This function takes a sorted linked list and seperates the nodes,which are having same size, into "only_repeated" linked list.
int create_repeated(SLL **files, SLL **only_repeated)
{
	SLL *cur_node = NULL, *prev_node = NULL, *old = NULL;
	SLL *temp = *files;
	int count = 0, exit_status = 0;
	
	while(temp){
		//old = temp;	
		prev_node = temp;
		temp = cur_node = temp->next ;
		
		//if sizes are equal, add prev_node to list
		if( cur_node && compare_size( cur_node->val, prev_node->val) == 0 ){
			//add node
			if(!count)
				if( -1 == (int)add_node(only_repeated,0,END,NULL) ){	//add_end
					exit_status = -1;
					goto EXIT;
				}

			attach_node(only_repeated,files,prev_node,old);
			count++;
		}
		//cur_node not matching with previous matched node, Add prev_node into list.
		else if (count){
			//add the last matching node to list
			attach_node(only_repeated,files,prev_node,old);
			count=0;
		}
		//if no match, delete the prev_node
		else{
			//delete non matching node, when no match has occured.
			delete_node( files, prev_node, YES, FREE, free_mem);
			count=0;
		}
	}

EXIT:
	return exit_status;

}


//read directory and return a linked list of filename sorted by size.
int read_directory(char* dir_name,SLL **files)
{

	DIR * dir=NULL;
	struct dirent *dir_info;
	struct stat file_info;
	struct file_data *temp;
	int filename_size=0,path_size=0;
	int err_val=0,exit_status=0; 
	char filename[256]={};

	dir=opendir(dir_name);
	if( !dir ) {
		perror("opendir");
		exit_status = -1;
		goto EXIT;
	}

	path_size = strlen(dir_name);

	//Read files names from directory
	while( (dir_info = readdir(dir)) ){

		filename_size=0;
		memset(filename,0,sizeof(filename));

		//get size of filename with Absolute path. 
		if( dir_name [ path_size - 1 ] != '/' )
			filename_size++;

		filename_size += strlen(dir_info->d_name);
		
		//Merge filename with absolute path
		strcpy(filename,dir_name);
		if( dir_name[path_size-1] != '/' )
			strcat(filename,"/");
		strcat(filename,dir_info->d_name);

		//Get file info	
		err_val = stat(filename,&file_info);
		if(err_val == -1){
			printf("%s : %s",dir_info->d_name,strerror(errno));
		}

		//If Regular files, create a node in linked list "files"
		if( file_info.st_mode & S_IFREG ){	

			temp = (struct file_data*) calloc(1,sizeof(struct file_data));	//create memory for structure
			if(!temp){
				printf("Unable to allocate the memory\n");	//use __FILE__,__LINE__
				exit_status = -1;
				//free_list(files,FREE,free_mem);	// free the files List.	
				goto EXIT_1;
			}

		
			//Create memory for filenames,
			temp->name = calloc(1,filename_size+path_size+2);	// Allocate size for file name
			if(!temp->name){
				printf("Calloc failed\n");
				exit_status = -1;
				//free_list(files,FREE,free_mem);	// free the files List.
				goto EXIT_1;
			}


			//fill the data structure
			temp->size = file_info.st_size;
			strcpy(temp->name,filename);

			if(-1 == (int) add_node(files,temp,SORT,compare_size) ){
				//free_list(files,FREE,free_mem);	// free the files List.
				exit_status = -1;
				goto EXIT_1;	
			}
		}//
	}

	//If error in reading diectory
	if(!dir_info && errno ){
		perror("readdir");
		exit_status = -1;
		goto EXIT_1;
	}

EXIT_1:
	closedir(dir);
EXIT:
	return exit_status;

}



//Print the repeated array (nested linked list) 
void print_repeated(SLL *repeated,void (*print_files)(void *ptr))
{
	SLL *ptr;
	while(repeated){
		ptr = (SLL*)repeated->val;
	//	printf("---\n");
		while(ptr ){
			print_files(ptr->val);
			ptr=ptr->next;
		}
		repeated=repeated->next;
	}

}


//function to print the structure data
void print_files(void *arg)
{
	struct file_data *ptr = (struct file_data*) arg;
	printf("size: %d\tfile: %s\n",ptr->size,ptr->name);
}

void free_mem(void *addr )
{
	struct file_data *ptr=(struct file_data *)addr ;
	if(ptr && ptr->name){
		free(ptr->name);
	}
}

int compare_size(void* data1, void* data2)
{
	struct file_data * p1 = (struct file_data*)data1;
	struct file_data * p2 = (struct file_data*)data2;
	if(p1->size == p2->size)
		return 0;
	else if (p1->size > p2->size)
		return 1;
	else // (p1->size < p2->size)
		return -1;
}


