/* 
 * tsh - A tiny shell program with job control
 * 
 * <Varun Jhaveri> 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define MAXARGS     128   /* max args on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXJID    1<<16   /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/* 
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;      /* defined in libc */
char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
int verbose = 0;            /* if true, print additional output */
int nextjid = 1;            /* next job ID to allocate */
char sbuf[MAXLINE];         /* for composing sprintf messages */
pid_t mainpid;


struct job_t {              /* The job struct */
    pid_t pid;              /* job PID */
    int jid;                /* job ID [1, 2, ...] */
    int state;              /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE];  /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv); 
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs); 
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid); 
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid); 
int pid2jid(pid_t pid); 
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != 255) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
            usage();
	}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}
  
/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/

/*void eval(char *cmdline) {
    // Holds pointers to each arg 
    char *argv[MAXARGS]; 
    // Holds modified command line 
    char buf[MAXLINE]; 

    int bg;
    // Process ID
    pid_t pid;
    // Signal Mask 
    sigset_t mask; 
    // Previous Signal Mask 
    sigset_t prev; 

    // Copy the command line to a buffer
    strcpy(buf, cmdline);
    // Parse the command line and determine if it's a bg job 
    bg = parseline(cmdline, argv);

    // Ignoring the empty lines
    if (argv[0] == NULL) {
        return;   
    }

    //Initialize an empty signal set named mask 
    sigemptyset(&mask);
    // Add SIGSTP to the signal set mask 
    sigaddset(&mask,SIGTSTP);
    // Add SIGINT to the signal set mask
    sigaddset(&mask,SIGINT);
    // Add SIGCHILD to the signal set mask     
    sigaddset(&mask,SIGCHLD);
    

    //Deciding job state 
    int state;
    if(bg){
        state=BG;
    }
    else{
        state=FG;
    }

    //Checking for built-in command
    if (!builtin_cmd(argv)) {

        // Initialise an empty signal set named mask
	    sigemptyset(&mask);
        // Add SIGCHILD to the signal set mask 
        sigaddset(&mask, SIGCHLD);
        // Block SIGCHILD temporarily
        sigprocmask(SIG_BLOCK, &mask, &prev);
        // Forking a child process
        pid = fork();
        // Child Process
        if (pid<0){
            unix_error("Fork error");
        }
        else if (pid == 0) {  
            // Unblocking SIGCHILD 
            sigprocmask(SIG_SETMASK, &prev, NULL);
            // Set the child process group ID
            setpgid(0,0);  

            for(int c1=0;argv[c1] != NULL;c1++){
                if (!strcmp(argv[c1], ">")) {
                    int newstdout;
                    int file_flags = O_WRONLY | O_CREAT | O_TRUNC;
                    mode_t file_mode = S_IRWXU | S_IRWXG | S_IRWXO;
                    newstdout = open(argv[c1 + 1], file_flags, file_mode);

                    if(newstdout==-1){
                        perror("Error opening output file for redirection");
                        exit(EXIT_FAILURE);
                    }
                    close(STDOUT_FILENO);
                    dup2(newstdout, STDOUT_FILENO);
                    close(newstdout);
                    for(int c2 = c1; argv[c2] != NULL; c2++ ){
                        argv[c2] = NULL;
                    }
                }
                else if (!strcmp(argv[c1], "<")) {
                    int newstdout;
                    int file_flags = O_RDONLY;
                    newstdout = open(argv[c1 + 1], file_flags);
                    if (newstdout== -1){
                        perror("Error opening output file for redirection");
                        exit(EXIT_FAILURE);
                    }
                    close(STDIN_FILENO);
                    dup2(newstdout, STDIN_FILENO);
                    close(newstdout);
                    for(int c2 = c1; argv[c2] != NULL; c2++){
                        argv[c2] = argv[c2+1];
                    }
                }
                else if (!strcmp(argv[c1], "2>")){
                    int newstdout;
                    int file_flags = O_WRONLY | O_CREAT;
                    mode_t file_mode = S_IRWXU | S_IRWXG | S_IRWXO;
                    newstdout = open(argv[c1 + 1], file_flags, file_mode);
                    if(newstdout==-1){
                        perror("Error opening output file for redirection");
                        exit(EXIT_FAILURE);
                    }
                    close(STDERR_FILENO);
                    dup2(newstdout, STDERR_FILENO);
                    close(newstdout);
                    for(int c2 = c1; argv[c2] != NULL; c2++){
                        argv[c2] = NULL;
                    }
                }
                else if (!strcmp(argv[c1], ">>")) {
                    int newstdout;
                    int file_flags = O_WRONLY | O_CREAT | O_APPEND;
                    mode_t file_mode = S_IRWXU | S_IRWXG | S_IRWXO;
                    newstdout = open(argv[c1 + 1], file_flags, file_mode);
                    if(newstdout==-1){
                        perror("Error opening output file for redirection");
                        exit(EXIT_FAILURE);
                    }
                    close(STDOUT_FILENO);
                    dup2(newstdout, STDOUT_FILENO);
                    close(newstdout);
                    for (int c2 = c1; argv[c2] != NULL; c2++) {
                        argv[c2] = NULL;
                        }
                }
                else if (!strcmp(argv[c1], "|" )) {
                    int thePipe[2];
                    pid_t pipe_pid;

                    //if (pipe(thePipe) == -1){
                    //    perror("Error creating pipe");
                    //    exit(EXIT_FAILURE);
                    //    }

                    pipe_pid = fork();

                    if(pipe_pid == 0){
                        close(thePipe[0]);
                        dup2(thePipe[1],STDOUT_FILENO);
                        close(thePipe[1]);
                        for(int c2 = 1; argv[c2]!=NULL; c2++){
                            argv[c2] = NULL;
                        }
                    }
                    else if (pipe_pid > 0){
                        close(thePipe[0]);
                        dup2(thePipe[1], STDOUT_FILENO);
                        close(thePipe[1]);
                        for(int c2 = 1; argv[c2] != NULL; c2++){
                            argv[c2] = NULL;
                        }
                    }
                    else{
                        perror("Error creating pipe");
                        exit(EXIT_FAILURE);
                    }
                    }
                }}

            if (execvp(argv[0], argv) < 0) {
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        }
        else{
            // Add jobs to list
            addjob(jobs, pid, state, cmdline);
            // Unblock SIGCHILD 
            sigprocmask(SIG_SETMASK, &prev, NULL);
            if(!bg){
                // Wait for fg job to end
                waitfg(pid); 
            }
            else{
                printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline); 
            }

        }
    }*/

void eval(char *cmdline){
    //Holds pointers to each arg 
    char *argv[MAXARGS];
    // Holds modified command line
    char buf[MAXLINE]; 
    int bg;
    //process id           
    pid_t pid; 
    // signal mask
    sigset_t mask;
    // Previous Signal Mask 
    sigset_t prev; 

    // Copy the command line to a buffer    
    strcpy(buf, cmdline);
    // Parse the command line and determine if it's a bg job 
    bg = parseline(cmdline, argv);

    // Ignoring the empty lines
    if (argv[0] == NULL) {
    return;   
    }
    // Code commented below was interfering with functionality hence I commented it out, it was part of my assignment 3
    /*
    //Initialize an empty signal set named mask 
    sigemptyset(&mask);
    // Add SIGSTP to the signal set mask 
    sigaddset(&mask,SIGTSTP);
    // Add SIGINT to the signal set mask
    sigaddset(&mask,SIGINT);
    // Add SIGCHILD to the signal set mask     
    sigaddset(&mask,SIGCHLD);
    */
    //Deciding job state 
    int state;
    if(bg){
        state=BG;
    }
    else{
        state=FG;
    }
    //Checking for built-in command
    if (!builtin_cmd(argv)) {
        // Initialise an empty signal set named mask
        sigemptyset(&mask);
        // Add SIGCHILD to the signal set mask
        sigaddset(&mask, SIGCHLD);
        // Block SIGCHILD temporarily
        sigprocmask(SIG_BLOCK, &mask, NULL);
        //pid = fork();
        if ((pid = fork()) == 0){
            // Set the child process group ID
            setpgid(0, 0);
            // Unblocking SIGCHILD 
            sigprocmask(SIG_UNBLOCK, &mask, NULL);

            // Redirection and pipe operators
            int i = 0;
            int j;
            while (argv[i] != NULL) {
                if (!strcmp(argv[i], "<")){
                    int file;
                    int file_flags = O_RDONLY;
                    file = open(argv[i + 1], file_flags);
                    close(STDIN_FILENO);
                    dup2(file, STDIN_FILENO);
                    close(file);
                    for(j=1;argv[j]!=NULL;j++){
                        argv[j] = argv[j+1];
                    }
                    
                
                    }
                else if (!strcmp(argv[i], ">")) {  
                    int file;
                    int file_flags = O_WRONLY | O_CREAT | O_TRUNC;
                    mode_t file_mode = S_IRWXU | S_IRWXG | S_IRWXO;
                    file = open(argv[i + 1], file_flags, file_mode);
                    close(STDOUT_FILENO);
                    dup2(file, STDOUT_FILENO);
                    close(file);
                    for(j=i;argv[j]!=NULL;j++){
                        argv[j]=NULL;
                    }
                    
                }
                else if (!strcmp(argv[i], ">>")) {  
                    int file;
                    int file_flags = O_WRONLY | O_CREAT | O_APPEND;
                    mode_t file_mode = S_IRWXU | S_IRWXG | S_IRWXO;
                    file = open(argv[i + 1], file_flags, file_mode);   
                    close(STDOUT_FILENO);
                    dup2(file, STDOUT_FILENO);
                    close(file);
                    for(j=i;argv[j]!=NULL;j++){
                        argv[j]=NULL;
                    }
                }
                else if (!strcmp(argv[i], "2>")) {
                    int file;
                    int file_flags = O_WRONLY | O_CREAT;
                    mode_t file_mode = S_IRWXU | S_IRWXG | S_IRWXO;
                    file = open(argv[i + 1], file_flags, file_mode);      
                    close(STDERR_FILENO);
                    dup2(file, STDERR_FILENO);
                    close(file);
                    for(j=i;argv[j]!=NULL;j++){
                        argv[j]=NULL;
                    }
                }
                else if (!strcmp(argv[i], "|")) {      
                    int fd[2];
                    pid_t pipe_pid = fork();
                    pipe(fd);
                    if (pipe_pid < 0) {
                        perror("Error creating pipe");
                        exit(EXIT_FAILURE);
                    }
                    else if (pipe_pid > 0) {
                        // Parent process (write to pipe)
                        close(fd[0]);
                        close(STDOUT_FILENO);
                        dup2(fd[1], STDOUT_FILENO);
                        close(fd[1]);
                    }
                    else {
                        // Child process (read from pipe)
                        close(fd[1]);
                        close(STDIN_FILENO);
                        dup2(fd[0], STDIN_FILENO);
                        close(fd[0]);
                    }

                    // Set remaining elements in argv to NULL
                    for (int j = i; argv[j] != NULL; j++) {
                        argv[j] = NULL;
                    }

            
                }
                i += 1;
        }

            if (execve(argv[0], argv, environ) < 0) {
            printf("%s: Command not found.\n", argv[0]);
            exit(0);
            }}
        else{
            // Add jobs to list
            addjob(jobs, pid, state, cmdline);
            // Unblock SIGCHILD 
            sigprocmask(SIG_SETMASK, &prev, NULL);
            if(!bg){
                // Wait for fg job to end
                waitfg(pid); 
            }
            else{
                printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline); 
            }

        }
    
    }}



/* 
 * parseline - Parse the command line and build the argv array.
 * 
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.  
 */
int parseline(const char *cmdline, char **argv)
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'') {
	buf++;
	delim = strchr(buf, '\'');
    }
    else {
	delim = strchr(buf, ' ');
    }

    while (delim) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* ignore spaces */
	       buf++;

	if (*buf == '\'') {
	    buf++;
	    delim = strchr(buf, '\'');
	}
	else {
	    delim = strchr(buf, ' ');
	}
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* ignore blank line */
	return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0) {
	argv[--argc] = NULL;
    }
    return bg;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 */

int builtin_cmd(char **argv) {
    // Compare the command with built-in commands
    if (strcmp(argv[0], "jobs") == 0) {
        // Execute the 'jobs' command
        listjobs(jobs);
        return 1;
    } else if (strcmp(argv[0], "bg") == 0 || strcmp(argv[0], "fg") == 0) {
        // Execute the 'bg' or 'fg' command
        do_bgfg(argv);
        return 1;
    } else if (strcmp(argv[0], "quit") == 0) {
        // Execute the 'quit' command
        exit(0);
        return 1;
    } else {
        // Not a built-in command
        return 0;
    }
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
    
void do_bgfg(char **argv) {
    // Check if the required argument is missing
    if (argv[1] == NULL) {
        printf("%s command requires PID or %%jobid argument", argv[0]);
        return;
    }

    // Determine if the argument is a job ID (starts with '%')
    int isJobID = (argv[1][0] == '%');
    int value;

    // Extract the numeric value from the argument
    if (sscanf(&argv[1][isJobID], "%d", &value) != 1) {
        printf("%s: argument must be a PID or %%jobid", argv[0]);
        return;
    }

    // Get the job based on whether it's a job ID or process ID
    struct job_t *job = isJobID ? getjobjid(jobs, value) : getjobpid(jobs, value);

    // Check if the job or process exists
    if (job == NULL) {
        printf("%s: No such %s", argv[0], isJobID ? "job" : "process");
        return;
    }

    // Set the state of the job based on the command ('fg' or 'bg')
    (*job).state = (strcmp(argv[0], "fg") == 0) ? FG : BG;

    // Send SIGCONT signal to the process group to resume execution
    if (kill(-(*job).pid, SIGCONT) < 0) {
        unix_error("Kill error");
    }

    // Print information for background job or wait for foreground job to finish
    if (strcmp(argv[0], "bg") == 0) {
        printf("[%d] (%d) %s", (*job).jid, (*job).pid, (*job).cmdline);
    } else {
        waitfg((*job).pid);
    }
}

     
/* 
 * waitfg - Block until process pid is no longer the foreground process
 */

void waitfg(pid_t pid)
{   

    // set of no signals
    sigset_t prev;
    sigemptyset(&prev);

    // if its in the foreground, wait until signal recieved
    while(fgpid(jobs) == pid){
        sigsuspend(&prev);
    }

    return;
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */

void sigchld_handler(int sig) 
{
    // save old err num
    int oldErr = errno;
    // keep pid
    pid_t pid;
    // initialize the status
    int status;

    // go until invalid PID 
    while((pid = waitpid(-1, &status, WUNTRACED | WNOHANG)) > 0){ 
		struct job_t *job = getjobpid(jobs, pid); //Get the job pid         
        // if terminated, print and delete
        if(WIFSIGNALED(status)){ 
            // save the job id and process id
            int jobT = job->jid;
            pid_t jobP = job->pid;
            printf("Signal %d terminated job %d %d", WTERMSIG(status), jobT, jobP);
            deletejob(jobs, pid);
        }
        // print what stopped, stop
        else if(WIFSTOPPED(status)){ 
            job->state = ST; 
            // save the job id and process id
            int jobT = job->jid;
            pid_t jobP = job->pid;
            printf("Signal %d stopped job %d %d", WSTOPSIG(status), jobT, jobP);
        }
        // otherwise, delete normally
        else if(WIFEXITED(status)){ 
            deletejob(jobs, pid);
        }
    }
    // reset old err num
    errno = oldErr; 
    return;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */

void sigint_handler(int sig) 
{
    // kill if foreground job
    if(fgpid(jobs) != 0){ 
        kill(-fgpid(jobs), SIGINT); 
    }
    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */

void sigtstp_handler(int sig) 
{
    // stop if foreground job
    if(fgpid(jobs) != 0){ 
        kill(-fgpid(jobs), SIGTSTP); 
    }
    return;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs) 
{
    int i, max=0;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid > max)
	    max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) 
{
    int i;
    
    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == 0) {
	    jobs[i].pid = pid;
	    jobs[i].state = state;
	    jobs[i].jid = nextjid++;
	    if (nextjid > MAXJOBS)
		nextjid = 1;
	    strcpy(jobs[i].cmdline, cmdline);
  	    if(verbose){
	        printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
	}
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == pid) {
	    clearjob(&jobs[i]);
	    nextjid = maxjid(jobs)+1;
	    return 1;
	}
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].state == FG)
	    return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid)
	    return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid) 
{
    int i;

    if (jid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid == jid)
	    return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid) {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs) 
{
    int i;
    
    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid != 0) {
	    printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
	    switch (jobs[i].state) {
		case BG: 
		    printf("Running ");
		    break;
		case FG: 
		    printf("Foreground ");
		    break;
		case ST: 
		    printf("Stopped ");
		    break;
	    default:
		    printf("listjobs: Internal error: job[%d].state=%d ", 
			   i, jobs[i].state);
	    }
	    printf("%s", jobs[i].cmdline);
	}
    }
}
/******************************
 * end job list helper routines
 ******************************/


/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}



