
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"

#define NULL 0

void eval(char *cmdline);
int builtin_command(char** argv);
int parseline(char* buf , char ** argv);
void ShowComplete(char *command);
void ShowAllProcess();
void OpenPM();
void ShowPMHelp();
void ShowProcABC();

// global variables
char current_dir[64] = "/";
char current_user[16] = "root";
/*****************************************************************************
 *                               kernel_main
 *****************************************************************************/
/**
 * jmp from kernel.asm::_start.
 *
 *****************************************************************************/
PUBLIC int kernel_main()
{
	disp_str("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
		 "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

	int i, j, eflags, prio;
        u8  rpl;
        u8  priv; /* privilege */

	struct task * t;
	struct proc * p = proc_table;

	char * stk = task_stack + STACK_SIZE_TOTAL;

	for (i = 0; i < NR_TASKS + NR_PROCS; i++,p++,t++) {
		if (i >= NR_TASKS + NR_NATIVE_PROCS) {
			p->p_flags = FREE_SLOT;
			continue;
		}

	        if (i < NR_TASKS) {     /* TASK */
                        t	= task_table + i;
                        priv	= PRIVILEGE_TASK;
                        rpl     = RPL_TASK;
                        eflags  = 0x1202;/* IF=1, IOPL=1, bit 2 is always 1 */
			prio    = 15;
                }
                else {                  /* USER PROC */
                        t	= user_proc_table + (i - NR_TASKS);
                        priv	= PRIVILEGE_USER;
                        rpl     = RPL_USER;
                        eflags  = 0x202;	/* IF=1, bit 2 is always 1 */
			prio    = 5;
                }

		strcpy(p->name, t->name);	/* name of the process */
		p->p_parent = NO_TASK;

		if (strcmp(t->name, "INIT") != 0) {
			p->ldts[INDEX_LDT_C]  = gdt[SELECTOR_KERNEL_CS >> 3];
			p->ldts[INDEX_LDT_RW] = gdt[SELECTOR_KERNEL_DS >> 3];

			/* change the DPLs */
			p->ldts[INDEX_LDT_C].attr1  = DA_C   | priv << 5;
			p->ldts[INDEX_LDT_RW].attr1 = DA_DRW | priv << 5;
		}
		else {		/* INIT process */
			unsigned int k_base;
			unsigned int k_limit;
			int ret = get_kernel_map(&k_base, &k_limit);
			assert(ret == 0);
			init_desc(&p->ldts[INDEX_LDT_C],
				  0, /* bytes before the entry point
				      * are useless (wasted) for the
				      * INIT process, doesn't matter
				      */
				  (k_base + k_limit) >> LIMIT_4K_SHIFT,
				  DA_32 | DA_LIMIT_4K | DA_C | priv << 5);

			init_desc(&p->ldts[INDEX_LDT_RW],
				  0, /* bytes before the entry point
				      * are useless (wasted) for the
				      * INIT process, doesn't matter
				      */
				  (k_base + k_limit) >> LIMIT_4K_SHIFT,
				  DA_32 | DA_LIMIT_4K | DA_DRW | priv << 5);
		}

		p->regs.cs = INDEX_LDT_C << 3 |	SA_TIL | rpl;
		p->regs.ds =
			p->regs.es =
			p->regs.fs =
			p->regs.ss = INDEX_LDT_RW << 3 | SA_TIL | rpl;
		p->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;
		p->regs.eip	= (u32)t->initial_eip;
		p->regs.esp	= (u32)stk;
		p->regs.eflags	= eflags;

		p->ticks = p->priority = prio;
		p->runned_times = 0;
		p->current_priority = p->priority;
		p->id = i;

		p->p_flags = 0;
		p->p_msg = 0;
		p->p_recvfrom = NO_TASK;
		p->p_sendto = NO_TASK;
		p->has_int_msg = 0;
		p->q_sending = 0;
		p->next_sending = 0;

		for (j = 0; j < NR_FILES; j++)
			p->filp[j] = 0;

		stk -= t->stacksize;
	}

	k_reenter = 0;
	ticks = 0;

	p_proc_ready	= proc_table;

	init_clock();
    init_keyboard();

	restart();

	while(1){}
}


/*****************************************************************************
 *                                get_ticks
 *****************************************************************************/
PUBLIC int get_ticks()
{
	MESSAGE msg;
	reset_msg(&msg);
	msg.type = GET_TICKS;
	send_recv(BOTH, TASK_SYS, &msg);
	return msg.RETVAL;
}


/**
 * @struct posix_tar_header
 * Borrowed from GNU `tar'
 */
struct posix_tar_header
{				/* byte offset */
	char name[100];		/*   0 */
	char mode[8];		/* 100 */
	char uid[8];		/* 108 */
	char gid[8];		/* 116 */
	char size[12];		/* 124 */
	char mtime[12];		/* 136 */
	char chksum[8];		/* 148 */
	char typeflag;		/* 156 */
	char linkname[100];	/* 157 */
	char magic[6];		/* 257 */
	char version[2];	/* 263 */
	char uname[32];		/* 265 */
	char gname[32];		/* 297 */
	char devmajor[8];	/* 329 */
	char devminor[8];	/* 337 */
	char prefix[155];	/* 345 */
	/* 500 */
};


/*****************************************************************************
 *                            username & password
 *****************************************************************************/
struct user {
	char username[10];
	char password[10];
};

struct user users[3];

/*****************************************************************************
 *                                untar
 *****************************************************************************/
/**
 * Extract the tar file and store them.
 *
 * @param filename The tar file.
 *****************************************************************************/
void untar(const char * filename)
{
	printf("[extract `%s'\n", filename);
	int fd = open(filename, O_RDWR);
	assert(fd != -1);

	char buf[SECTOR_SIZE * 16];
	int chunk = sizeof(buf);

	while (1) {
		read(fd, buf, SECTOR_SIZE);
		if (buf[0] == 0)
			break;

		struct posix_tar_header * phdr = (struct posix_tar_header *)buf;

		/* calculate the file size */
		char * p = phdr->size;
		int f_len = 0;
		while (*p)
			f_len = (f_len * 8) + (*p++ - '0'); /* octal */

		int bytes_left = f_len;
		int fdout = open(phdr->name, O_CREAT | O_RDWR);
		if (fdout == -1) {
			printf("    failed to extract file: %s\n", phdr->name);
			printf(" aborted]");
			return;
		}
		printf("    %s (%d bytes)\n", phdr->name, f_len);
		while (bytes_left) {
			int iobytes = min(chunk, bytes_left);
			read(fd, buf,
			     ((iobytes - 1) / SECTOR_SIZE + 1) * SECTOR_SIZE);
			write(fdout, buf, iobytes);
			bytes_left -= iobytes;
		}
		close(fdout);
	}

	close(fd);

	printf(" done]\n");
}

/*****************************************************************************
 *                                shabby_shell
 *****************************************************************************/
/**
 * A very very simple shell.
 *
 * @param tty_name  TTY file name.
 *****************************************************************************/

void shabby_shell(const char * tty_name)
{
	int fd_stdin  = open(tty_name, O_RDWR);
	assert(fd_stdin  == 0);
	int fd_stdout = open(tty_name, O_RDWR);
	assert(fd_stdout == 1);

	char rdbuf[128];
	clear();

	//boot_animation();
	while (1)
	{
		printf("%s@mintos ", current_user);
		printf("%s", current_dir);
		write(1, "> ", 2);

		int r;
		if ((r = read(0, rdbuf, 70)) == 0)
			continue;
		else
		{
			rdbuf[r] = 0;
		}

		eval(rdbuf);
	}
	close(1);
	close(0);
}

void eval(char* command)
{
	char *argv[128]; /* argv for execve() */
    int bg;              /* should the job run in bg or fg? */
    int pid;           /* process id */

    /* parse command line */
	bg = parseline(command, argv);

    if (argv[0] == 0)
		return;   /* ignore empty lines */


	int fd = open(argv[0], O_RDWR);
	if (fd == -1)
	{
		if (!builtin_command(argv))
		{
			printf("%s: Command not found.\n", argv[0]);
			ShowComplete(command);
		}
	}
	else
	{
		if ((pid = fork()) == 0)
		{
			execv(argv[0], argv);
		}
		// else
		// {
		// 	int s;
		// 	wait(&s);
		// }
		if(!bg)
		{
			int s;
			wait(&s);
		}
		else
			printf("%d %s",pid,command);
	}
    return;
}

int builtin_command(char **argv )//todo
{
	if(!strcmp(argv[0],"clear"))
	{
		clear();
		return 1;
	}
	if(!strcmp(argv[0],"pm"))
	{
		OpenPM();
		return 1;
	}
	if(!strcmp(argv[0],"process"))
	{
		ShowAllProcess();
		return 1;
	}
	if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
		return 1;
	if(!strcmp(argv[0] , "ls"))
	{
		ls(current_dir);
		return 1;
	}
	if(!strcmp(argv[0] , "mkdir"))
	{
		char full_dirname[64];
		addTwoString(full_dirname, current_dir, argv[1]);
		// int pos = strlen(full_dirname);
		// full_dirname[pos] = '/';
		// full_dirname[pos + 1] = 0;
		int fd = mkdir(full_dirname);
		if (fd == -1) {
			printf("Cannot create directory\n");
			return 1;
		}

		printf("%s: directory create success\n", argv[1]);
		return 1;
	}
	if(!strcmp(argv[0] , "cd"))
	{
		/* relative path */
		char full_path[64];
		char tmp[64];
		if (!strcmp(argv[1], "..")) {
			char upper_path[64] = "/";
			int pos = 0;
			for (int n = strlen(current_dir) - 1; n > 0; n--) {
				if (current_dir[n - 1] == '/') {
					pos = n - 1;
					break;
				}
			}
			for (int n = 0; n < pos; n++) {
				upper_path[n] = current_dir[n];
			}

			memcpy(full_path, upper_path, 64);
		}
		else if (argv[1][0] != '/') {
			addTwoString(tmp, current_dir, argv[1]);
			memcpy(full_path, tmp, 64);
		}
		/* absolute path */
		else if (argv[1][0] == '/') {
			memcpy(full_path, argv[1], 64);
		}

		printf("%s\n", full_path);
		int fd = open(full_path, O_RDWR);
		if (fd == -1) {
			printf("Cannot change directory\n");
			return 1;
		}
		int pos = strlen(full_path);
		if (pos > 1) {
			full_path[pos] = '/';
			full_path[pos + 1] = 0;
		}
		memcpy(current_dir, full_path, 64);
		return 1;
	}
	if(!strcmp(argv[0] , "create"))
	{
		char full_filename[64];
		char tmp[64];
		if (argv[1][0] != "/") {
			addTwoString(tmp, current_dir, argv[1]);
			memcpy(full_filename, tmp, 64);
		}
		int fd = open(full_filename, O_CREAT | O_RDWR);

		if (fd == -1){
			printf("Cannot create file\n");
			return 1;
		}
		printf("%s: file create success\n", argv[1]);
		close(fd);

		return 1;
	}
	if(!strcmp(argv[0] , "rm"))
	{
		char full_path[64];
		char tmp[64];
		/* relative path */
		if (argv[1][0]!='/') {
				addTwoString(tmp, current_dir, argv[1]);
				memcpy(full_path, tmp, 64);
		}

		int result;
		result = unlink(full_path);
		if (result == 0)
		{
				printf("%s: file deleted!\n", argv[1]);
		}
		else
		{
				printf("Cannot delete file\n");
		}
		return 1;
	}
	if(!strcmp(argv[0] , "mv"))
	{
		/* get the destination */
		char full_path[64];
		char tmp[64];
		char tmp_2[64];
		  /* relative path */
		if (argv[2][0]!='/') {
			addTwoString(tmp, current_dir, argv[2]);
			addTwoString(tmp_2, tmp, argv[1]);
			memcpy(full_path, tmp_2, 64);
		}
		else {
			if (argv[1][0]!='/') {
				addTwoString(tmp_2, argv[2], argv[1]);
				memcpy(full_path, tmp_2, 64);
			}
			// TODO both absolute path situation
		}
		/* open the file */
		char full_filename[64];
		char filename_tmp[64];
			/* relative path */
		if (argv[1][0]!='/') {
				addTwoString(filename_tmp, current_dir, argv[1]);
				memcpy(full_filename, filename_tmp, 64);
		}
		else {
			memcpy(full_filename, argv[1], 64);
		}
		int fd  = open(full_filename, O_RDWR);
		if (fd == -1) {
			printf("Cannot locate file: %s\n", argv[1]);
			return 1;
		}
		/* get the content */
		char content[128] = "";
		int n = read(fd, content, 128);
		close(fd);
		content[n] = 0;
		/* create new file */
		int new_fd = open(full_path, O_CREAT);
		if (new_fd == -1) {
			printf("Cannot move file\n");
			return 1;
		}
		else {
			write(new_fd, content, n);
			close(new_fd);
			unlink(argv[1]);
		}
		return 1;
	}
	if(!strcmp(argv[0] , "add"))
	{
		char full_filename[64];
		char tmp[64];
		if (argv[1][0] != "/") {
			addTwoString(tmp, current_dir, argv[1]);
			memcpy(full_filename, tmp, 64);
		}
		int fd = open(full_filename, O_RDWR);
		if (fd == -1){
			printf("Cannot locate file: %s\n", argv[1]);
			return 1;
		}
		/* get the content */
		char content[128];
		int n = read(fd, content, 128);
		content[n] = 0;
		close(fd);

		printf("enter the new content to append\n");
		char rdbuf[128];
		int count = read(0, rdbuf, 128);
		rdbuf[count] = 0;

		char new_content[128] = "";
		addTwoString(new_content, content, rdbuf);

		fd = open(full_filename, O_RDWR);
		write(fd, new_content, strlen(new_content));
		close(fd);
		printf("content appended\n");
		return 1;
	}
	if(!strcmp(argv[0] , "pwd"))
	{
		printf("%s\n", current_dir);
		return 1;
	}
	if(!strcmp(argv[0] , "write"))
	{
		writeFile(argv[1]);
		return 1;
	}
	if(!strcmp(argv[0] , "open"))
	{
		openFile(argv[1]);
		return 1;
	}
	if(!strcmp(argv[0] , "ls"))
	{
		ls(current_dir);
		return 1;
	}
	else if(!strcmp(argv[0] , "reg"))
	{
		userRegister();
		return 1;
	}
  else if (!strcmp(argv[0], "login"))
	{
		userLogin(argv[1]);
		return 1;
	}
	else if(!strcmp(argv[0] , "help"))
	{
		help();
		return 1;
	}
	else if(!strcmp(argv[0], "quit"))
	{
		strcpy(current_user, "root");
		exit(0);  /* terminate shell */
	}
	else
		return 0;
}


int parseline(char *cmdline ,char** argv)
{
	char array[128]; /* holds local copy of command line */
    char *buf = array;   /* ptr that traverses command line */
    char *delim;         /* points to first space delimiter */
    int argc;            /* number of args */
    int bg;              /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf)] = ' ';  /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
		buf++;

    /* build the argv list */
    argc = 0;
	while ((delim = strchr(buf, ' ')))
	{
		argv[argc++] = buf;
		*delim = '\0';
		buf = delim + 1;
		while (*buf && (*buf == ' ')) /* ignore spaces */
			buf++;
    }
	argv[argc] = 0;

	for(int i = 0 ; i < argc ; i++)

    if (argc == 0)  /* ignore blank line */
		return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0)
		argv[--argc] = 0;

    return bg;
}

/*****************************************************************************
 *                                Init
 *****************************************************************************/
/**
 * The hen.
 *
 *****************************************************************************/
void Init()
{
	int fd_stdin  = open("/dev_tty0", O_RDWR);
	assert(fd_stdin  == 0);
	int fd_stdout = open("/dev_tty0", O_RDWR);
	assert(fd_stdout == 1);

	/* extract `cmd.tar' */
	untar("/cmd.tar");


	char * tty_list[] = {"/dev_tty0"};

	int i;
	for (i = 0; i < sizeof(tty_list) / sizeof(tty_list[0]); i++) {
		int pid = fork();
		if (pid != 0) { /* parent process */
		}
		else {	/* child process */
			close(fd_stdin);
			close(fd_stdout);

			shabby_shell(tty_list[i]);
			assert(0);
		}
	}

	while (1) {
		int s;
		int child = wait(&s);
		printf("child (%d) exited with status: %d.\n", child, s);
	}

	assert(0);
}


/*======================================================================*
                               TestA
 *======================================================================*/
void TestA()
{
	while(1);
	// for(;;)
	// {
	// 	disp_str("A");
	// 	milli_delay(300);
	// }
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestB()
{
	while(1);

	// for(;;)
	// {
	// 	disp_str("B");
	// 	milli_delay(900);
	// }
}

/*======================================================================*
                               TestC
 *======================================================================*/
void TestC()
{
	while(1);

	// for(;;)
	// {
	// 	disp_str("C");
	// 	milli_delay(1500);
	// }
}

/*****************************************************************************
 *                                panic
 *****************************************************************************/
PUBLIC void panic(const char *fmt, ...)
{
	int i;
	char buf[256];

	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char*)&fmt + 4);

	i = vsprintf(buf, fmt, arg);

	printl("%c !!panic!! %s", MAG_CH_PANIC, buf);

	/* should never arrive here */
	__asm__ __volatile__("ud2");
}

void clear()
{
	int i = 0;
	for (i = 0; i < 25; i++)
		printf("\n");
	// clear_screen(0,console_table[current_console].cursor);
    // console_table[current_console].crtc_start = console_table[current_console].orig;
    // console_table[current_console].cursor = console_table[current_console].orig;
}

/*****************************************************************************
 *                    below are help functions for file system
 *****************************************************************************/

 /*
 * create a file
 */

 void createFile(char * filename) {
	//  char * final_name;
	//  addTwoString(final_name, "/", filename);
  //  printf("%s\n", final_name);
	 printf("%s\n", filename);
	 int fd = open(filename, O_CREAT | O_RDWR);
	 printf("fd: %d\n", fd);

	 if (fd == -1){
		 printf("Cannot create file\n");
		 return;
	 }
	 close(fd);
	 return;
 }

 /*
 * write an existing file
 */

 void writeFile(char * filename) {
	 char tmp[64];
	 char full_path[64];
	 if (filename[0] != '/') {
		addTwoString(tmp, current_dir, filename);
		memcpy(full_path, tmp, 64);
	 }
	 int fd = open(full_path, O_RDWR);
	 if (fd == -1) {
		 printf("Cannot locate file: %s\n", filename);
	 	return;
	 }
	 printf("Enter the content: \n");
	 char rdbuf[128] = "";
	 int n = read(0, rdbuf, 128);
	 rdbuf[n] = 0;
	 write(fd, rdbuf, n);
	 printf("write finished!\n");
	 close(fd);
 }

 /*
 * open and read an existing file
 */

 void openFile(char * filename) {
	 char tmp[64];
	 char full_path[64];
	 if (filename[0] != '/') {
	 	addTwoString(tmp, current_dir, filename);
		memcpy(full_path, tmp, 64);
	 }
	 int fd = open(full_path, O_RDWR);
	 if (fd == -1) {
	 	 printf("Cannot find file: %s\n", filename);
		 return;
	 }

	 char content[128] = "";
	 int n = read(fd, content, 128);
	 content[n] = 0;
	 printf("%s\n", content);
	 close(fd);
 }

 /*
 @return 0 for success and -1 for fail
 */

 int userRegister() {
 	 printf("Enter username\n");
	 char rdbuf[10];
	 int n = read(0, rdbuf, 10);
	 rdbuf[n] = 0;
	 for (int i = 0; i < 3; i++) {
	   if (strcmp(users[i].username, rdbuf) == 0)  {
			 printf("username invalid\n");
			 return -1;
		 }
	 }

	 for (int i = 0; i < 3; i++) {
		 if (strcmp(users[i].username, "") == 0) {
			 strcpy(users[i].username, rdbuf);
			 printf("Enter password\n");
			 char rdbuf_pw[10];
			 int n = read(0, rdbuf_pw, 10);
			 rdbuf_pw[n] = 0;
			 strcpy(users[i].password, rdbuf_pw);
			 printf("%s\n", users[i].password);
			 return 0;
		 }
	 }
	 return -1;
 }

 /*
 @param username the username to login
 @return 0 for success and -1 for fail
 */

 int userLogin(char * username) {
	 for (int i = 0; i < 3; i++) {
		 if (strcmp(users[i].username, username) == 0) {
			 printf("Enter password to login\n");
			 char rdbuf_pw[10];
			 int n = read(0, rdbuf_pw, 10);
			 rdbuf_pw[n] = 0;
			 if (strcmp(users[i].password, rdbuf_pw) == 0) {
				 printf("Welcome, %s\n", username);
				 strcpy(current_user, username);
				 return 0;
			 }
		 }
	 }
	 printf("login denied.\n");
	 return -1;
 }


/*
* print the help function
*/

void help() {
	printf("**************************************                       **   \n");
	printf("*****************************                              *****  \n");
	printf("                                                          ******* \n");
	printf("                                 help                      *****  \n");
	printf("                                                            ***   \n");
	printf("*****************************                                 *   \n");
	printf("**************************************                            \n");
	printf("\n");
	printf("   ls                  		list the files and folders in current directory\n");
	printf("   pwd                  	show the current directory\n");
	printf("   cd      [dir]        	change the directory\n");
	printf("   mkdir   [filename]   	create a new folder in current directory\n");
	printf("   create  [filename]  	 	create a new file in current directory\n");
	printf("   open    [filename]   	open the file\n");
	printf("   write   [filename]   	write the file\n");
	printf("   add     [filename]   	add new content to the file\n");
	printf("   rm      [filename]   	remove the file\n");
	printf("   mv  [filename][desdir]   move the file to destination directory\n");
	printf("   login   [username]   	login with the username\n");
	printf("   reg                  	register user\n");
	printf("   game                 	play games\n");
	printf("   process              	show process info\n");
	printf("   pm                   	manage process\n");	
	printf("   quit                 	quit the shell\n");
}

 double CaclSimilarity(const char *command ,const char *target)
 {
	int length = strlen(command);
	int count = 0;

	for(int i = 0 ; i < length ; i++)
	{
		if(command[i] == target[i])
			count++;
	}

	return (double) count / strlen(target);

 }

 void ShowComplete(char *command)
 {
	char resource[][10] = {"ls","help","mkdir","create","rm","quit","login","reg","open","write","cd","pwd",
						"game","process","pm"};

	int size = 15; //记得改
	double Score[size];
	int isFirst = 1;
	int isExist = 0;
	int temp = 0;

	for(int i = 0 ; i < size ; i++)
	{
		if(!strcmp_length(command,resource[i],strlen(command)))//是否完全相等
		{
			if(isFirst)
			{
				printf("Maybe you mean %s " , resource[i]);
				isFirst = 0;
				isExist++;
			}
			else
				printf("/ %s ", resource[i]);
		}
		else
		{
			Score[i] = CaclSimilarity(command , resource[i]);
		}
	}

	if(!isExist)
	{
		for(int i = 1 ; i < size ; i++)
			if(Score[temp] < Score[i])
				temp = i;
		if(Score[temp] > 0.5)
			printf("Maybe you mean %s ?" , resource[temp]);
	}


	printf("\n\n");

 }

 void ShowAllProcess()
 {
	 int i;
	 printf("=============================================================================\n");
	 printf(" id | name | spriority | running?\n");
	 //进程号，进程名，优先级，是否是系统进程，是否在运行
	 printf("-----------------------------------------------------------------------------\n");
	 for ( i = 0 ; i < NR_TASKS + NR_PROCS ; ++i )//逐个遍历
	 {
		 if ( proc_table[i].priority == 0 || !strcmp(proc_table[i].name , "INIT_9") )
		 	continue;//系统资源跳过*/
		printf(" %2d %6s      %2d        yes\n", proc_table[i].id ,proc_table[i].name, proc_table[i].priority);
	 }
	 printf("=============================================================================\n");
 }

void OpenPM()
 {
	 char rdbuf[128];
	 int r, id, i;
	 ShowPMHelp();

	 while (1)
	 {
		 ShowProcABC();

		 printf("\n>>> ");
		 r = read(0, rdbuf, 70);
		 rdbuf[r] = 0;
		 id = rdbuf[r - 1] - '0';
		 for (i = 0; i < r; ++i)
		 {
			 if (rdbuf[i] == ' ')
			 {
				 rdbuf[i] = 0;
				 break;
			 }
		 }

		 if (strcmp(rdbuf, "exit") == 0)
		 {
			 return;
		 }

		 if (strcmp(rdbuf, "help") == 0)
		 {
			 ShowPMHelp();
			 return ;
		 }

		 if(id < 6 || id > 8)
		 {
			 printf("error id!\n");
			 continue;
		 }

		 if (strcmp(rdbuf, "pause") == 0)
		 {
			 proc_table[id].p_flags = 1;
		 }
		 else if (strcmp(rdbuf, "resume") == 0)
		 {
			 proc_table[id].p_flags = 0;
		 }
		 else if (strcmp(rdbuf, "elevate") == 0)
		 {
			 proc_table[id].priority *= 2;
		 }
		 else if (strcmp(rdbuf, "kill") == 0)
		 {
			 proc_table[id].p_flags = -1;
		 }
		 else if (strcmp(rdbuf, "") == 0)
		 {
			 continue;
		 }
		 else
		 {
			 printf("\nUnknown Command");
		 }
	 }
 }

 void ShowProcABC()
 {
	 int i;
	 printf("\n=============================================================================\n");
	 printf("      id          |    name       |  priority    | running?\n");
	 printf("-----------------------------------------------------------------------------\n");
	 for (i = NR_TASKS; i < NR_TASKS + NR_PROCS; ++i)
	 {
		 if (proc_table[i].p_flags < 0)
		 {
			 continue;
		 }
		 if(proc_table[i].id >= 6 && proc_table[i].id <= 8)
			printf("       %d               %s           %2d          %s  \n",
				proc_table[i].id, proc_table[i].name, proc_table[i].priority,
				(proc_table[i].p_flags == 0 ? "running" : "paused"));
	 }
 }

void ShowPMHelp()
 {
	 printf("=============================================================================\n");
	 printf(" pause [id]:       pause a process                                         \n");
	 printf(" resume [id]:      resume a process                                         \n");
	 printf(" elevate [id]:     higher a process's priority                              \n");
	 printf(" kill [id]:        kill a process                                         \n");
	 printf(" exit:             exit                                                   \n");
	 printf("=============================================================================\n");
 }
