#include <stdio.h> // For handling standard input/output for user interactions.
#include <string.h> // For calling built-in string manipulation functions.
#include <stdlib.h> // Library to help manage the process creation, and execution.
#include <unistd.h> // For built-in functions such as getcwd().
#include <sys/types.h> // For the wait() function.
#include <sys/wait.h> // For the wait() function.
#include <readline/readline.h> // Used to auto-complete text on shell prompt.
#include <readline/history.h> // Handles the storage of history for commands.

/*--------------------------------------------------------------------------*/

// Function definitions:

int userInputTake(char* userInput); // Should take user input from the shell and store.
int execFlagProcess(char* userInput, char** userInpParsed, char** userInpParsedPiped, char** userInpParsedRedirect); // Function should determine what type of execution is required, a normal, piped or a built-in command execution?
int cmdRedirectParse(char* userInput, char** userInpEdit); // Checks if there is a redirect '> or >>' character present, and splits up the user input into two different commands (before and after redirect).
int cmdPipeParse(char* userInput, char** userInpEdit); // Checks if there is a pipe '|' character present, and splits the user input up into two different commands (before and after the pipe).
int cmdSpaceParse(char* userInput, char** userInpParsed); // Takes the user input and studies it for spaces in the edited command(s). Anytime a space is present, it will strip it and store it back into the userInpParse pointer of a pointer array.
int builtinCmds(char** userInpParsed); // Function executes built-in commands that were necessary as they could not be accessed via the other method (exec()) of execution. Built-in commands: cd and exit.
void execCmd(char** userInpParsed); // Executes the user input command if there was no pipe to handle.
void execCmdPiped(char** userInpParsed, char** userInpParsedPiped); // Executes the user input command if there was a pipe to handle.
void execCmdRedirect(char** userInpParsed, char** userInpParsedRedirect); // COMMENT later
/*--------------------------------------------------------------------------*/

// Take user input:
int userInpTake(char* userInput)
{
  char *inpBuffer;
  size_t inpBufferSize = 1024;
  
  char *uname = getenv("USER"); // Gets the current user's name from the environmental variables under the entry "USER".
  char currDir[1024]; // An array to store the bytes of every character in the directory path.
  int userInpFlag; // Determines if the user input is good or not. 0 - Good, 1 - User input was not there.

  getcwd(currDir, sizeof(currDir)); // Copies an absolute pathname of the current directory we are in into the buffer "currDir" by allocating the proper length size to the buffer. Essentially, how much space will be used within the buffer in terms of length size.

  printf("%s: %s\n", uname, currDir); // Username: Current Directory display.
  
  // Input method 1:
  inpBuffer = readline("> "); // Reads a line from the terminal and returns it into the input buffer pointer. Any input read here is stored somewhere in memory that input buffer points to.
  
  // Next check if the input from the user is there or not.
  if(strlen(inpBuffer) != 0)
  {
    add_history(inpBuffer); // Built-in function to the readline/history.h library. This helps add the commands in a separate space in memory to be easily accessible by keys later.

    strcpy(userInput, inpBuffer); // Takes whatever is in inpBuffer pointer and copies it over to the userInput memory space. This points back to the array in main which is then populated by this pointer sending the characters to their respective memory location. The copy continues until the null terminator is hit from the source buffer.
    userInpFlag = 0;
  }
  else
  {
    userInpFlag = 1;
  }

  return userInpFlag; // Send the flag value back to main to decide what happens next.
}

/*--------------------------------------------------------------------------*/

// Determines the flag status of the execution of command:
int execFlagProcess(char* userInput, char** userInpParsed, char** userInpParsedPiped, char** userInpParsedRedirect)
{
  char* userInpEdit[2]; // This pointer array will hold the memory locations to wherever the various commands begin.
  /*
    For example:
    - Without pipe: cmdEdit[0] holds the memory address of 0x1000 where the command begins.
    - With pipe: cmdEdit[0] holds the memory address of 0x1000 where the first command starts, then it populates cmdEdit[1] with the memory address of where the next command begins in memory based on automatic allocation by the system.
   */

  int pipeFlag; // Is there a pipe present in the command? 0 - No, 1 - Yes.
  int redirectFlag; // Is there a redirect present in the command? 0 - No, 1 - Yes. Should not work with the piping functionality as that is buggy.
  int typeOfExecFlag = 0; // What type of execution should take place in main()? 0 - Builtin, 1 - Normal, 2 - Piped.

  redirectFlag = cmdRedirectParse(userInput, userInpEdit);
  //printf("Redirect Flag - %d\n", redirectFlag); // TEST
  //pipeFlag = cmdPipeParse(userInput, userInpEdit); // Is there a pipe symbol in the user provided command?

  if(redirectFlag == 0)
  {
    pipeFlag = cmdPipeParse(userInput, userInpEdit); // Is there a pipe symbol in the user provided command?
  }
  else if(redirectFlag == 1)
  {
    pipeFlag = 0;
  }

  if(redirectFlag == 1)
  {
    cmdSpaceParse(userInpEdit[0], userInpParsed);
    cmdSpaceParse(userInpEdit[1], userInpParsedRedirect);

    typeOfExecFlag = 3;
  }
  else if(pipeFlag == 1 && redirectFlag == 0)
  {
    // There is a pipe in the command, so do the following.
    cmdSpaceParse(userInpEdit[0], userInpParsed); // Send in the first command to parse out the space and store it back into userInpParsed's memory location. - Stores the first command after space parsing.
    cmdSpaceParse(userInpEdit[1], userInpParsedPiped); // Send in the second command to parse out the space and store it back into the userInpParsedPiped's memory location. - Stores the second command after space parsing.
    typeOfExecFlag = 2;
  }
  else
  {
    // There is no pipe symbol in the user input.
    cmdSpaceParse(userInput, userInpParsed); // Send in the user input command to parse out the space and store it back into the userInpParsed's memory location for later execution.
    typeOfExecFlag = 1;
  }

  // The command that is stored at the beginning memory address of userInpParsed is ran through my builtin commands to see if it exists there. If it does, execute, else return another value and check against the if condition.
  int builtInFlag = builtinCmds(userInpParsed);
    
  if(builtinCmds(userInpParsed) == 1)
  {
    typeOfExecFlag = 0;
  }

  return typeOfExecFlag; // Go back to main with this newly found value.
}

/*--------------------------------------------------------------------------*/

// Determines if there is a redirect symbol in the user input command.
int cmdRedirectParse(char* userInput, char** userInpEdit)
{
  int i;
  int redirectFlag; // 0 - No redirect, 1 - There is a redirect.

  for(i = 0; i < 2; i++)
  {
    // Loops a total 2 times, the number of elements for userInpEdit, and checks for the pipe symbol...

    userInpEdit[i] = strsep(&userInput, ">>"); // The strsep function goes through the dereferenced memory address of userInput, and cycles through every character until it either hits the null terminator, or the specified delimeter/symbol. It will store the beginning pointer address in the first element, then it will continue to read from the second beginning pointer address, until the null terminator is hit.
    
    if(userInpEdit[i] == NULL)
    {
      break; // Says that no point in going on with the loop if the current element of the pointer array is empty, that means that there are no more commands to look for. Exit the loop.
    }
  }

  if(userInpEdit[1] == NULL)
  {
    redirectFlag = 0; // There was no second command read, meaning that there was no redirect.
  }
  else
  {
    redirectFlag = 1; // A second command is indeed present.
  }

  //printf("Redirect first check: %d\n", redirectFlag); // TEST
  
  return redirectFlag;
}

/*--------------------------------------------------------------------------*/

// Determines if there is a pipe symbol in the user input command.
int cmdPipeParse(char* userInput, char** userInpEdit)
{
  int i;
  int pipeFlag; // 0 - No pipe, 1 - There is a pipe.

  for(i = 0; i < 2; i++)
  {
    // Loops a total 2 times, the number of elements for userInpEdit, and checks for the pipe symbol...

    userInpEdit[i] = strsep(&userInput, "|"); // The strsep function goes through the dereferenced memory address of userInput, and cycles through every character until it either hits the null terminator, or the specified delimeter/symbol. It will store the beginning pointer address in the first element, then it will continue to read from the second beginning pointer address, until the null terminator is hit.
    
    if(userInpEdit[i] == NULL)
    {
      break; // Says that no point in going on with the loop if the current element of the pointer array is empty, that means that there are no more commands to look for. Exit the loop.
    }
  }

  if(userInpEdit[1] == NULL)
  {
    pipeFlag = 0; // There was no second command read, meaning that there was no pipe.
  }
  else
  {
    pipeFlag = 1; // A second command is indeed present.
  }

  return pipeFlag;
}

/*--------------------------------------------------------------------------*/

// Function parses out the empty spaces from command so that when it is read for execution, there are no issues. Otherwise, the child process will not be able to execute the command properly.
int cmdSpaceParse(char* userInput, char** userInpParsed)
{
  int i;

  // Loop handles the individual elements at the various pointer addresses. We are looking for blank spaces to get rid of...
  for(i = 0; i < 200; i++)
  {
    userInpParsed[i] = strsep(&userInput, " "); // The strsep function goes through the dereferenced memory address of userInput, and cycles through every character until it either hits the null terminator, or the specified delimeter/symbol. It will store the beginning pointer address in the first element, then it will continue to read from the second beginning pointer address, until the null terminator is hit.

    if(userInpParsed[i] == NULL)
    {
      break; // This means that we have reached the end of the command and no point in continuing.
    }

    if(strlen(userInpParsed[i]) == 0)
    {
      i--; // Meaning, if no address was stored at this element, BUT we haven't finished with the command, we need to go back one element to ensure that we don't skip over any elements in the pointer array and keep it sequential. Otherwise, it will cause problems during execution of the commands. 
    }
  }
}

/*--------------------------------------------------------------------------*/

// Function to execute built-in commands.
int builtinCmds(char** userInpParsed)
{
  char* cmdBuiltin[2];
  cmdBuiltin[0] = "cd";
  cmdBuiltin[1] = "exit";
  
  int i;
  int builtInCmdFlag = 0; // Flag to store the elemental position of the determined command from the array of commands.
  int cmdFlag; // Signifies if this was a built-in command or not. 0 - No, 1 - Yes.

  for(i = 0; i < 2; i++)
  {
    // Loops through the 2 elements of the commands array. Does a check against the provided user command to see if it is a built-in command.
    if(strcmp(userInpParsed[0], cmdBuiltin[i]) == 0)
    {
      builtInCmdFlag = i + 1; // If match found, save the position of i.
      break; // No longer any point in continuing, we have what we wanted.
    }
  }

  switch(builtInCmdFlag)
  {
  case 1: // cd
    chdir(userInpParsed[1]); // The element 1 stores the pointer address to the file path in memory.
    return 1;
  case 2: // Exit
    exit(0); // Exits the shell program.
  default:
    break;
  }

  return 0;
}

/*--------------------------------------------------------------------------*/

// Executes a user command that is not piped.
void execCmd(char** userInpParsed)
{
  pid_t childProcess = fork(); // Creating a child process to help execute our user's command.

  // If fork ever returns a value below 0, it means that there was a problem. Conducting a check for that below:
  if(childProcess < 0)
  {
    printf("Error in executing command - Child process could not be successfuly created. Please try again later...\n");
    return; // Goes back to main to begin the process anew. Ideal health of the child process for return value should be 0. Then execute the rest.
  }
  else if(childProcess == 0)
  {
    // Sending the user parsed input(s) to the OS so that it can convey the command to the kernel and display results.
    if(execvp(userInpParsed[0], userInpParsed) < 0)
    {
      printf("Error in executing command - Command was not successfully sent to the OS. Please try again later...\n"); // In case something went wrong.
    }
    
    exit(0); // Exits the child process and goes back to the parent process.
  }
  else
  {
    wait(NULL); // Waits for the child process to die if none of the conditions above are met.
    return; // Go back to main.
  }
}

/*--------------------------------------------------------------------------*/

// Executes a piped user command.
void execCmdPiped(char** userInpParsed, char** userInpParsedPiped)
{
  int fd[2]; // File descriptor to respresent 2 states for the pipe. 0 - Read, 1 - Write as per the file descriptor standards.
  /* 0 - Standard Input - STDIN_FILENO - stdin - Reading
     1 - Standard Output - STDOUT_FILENO - stdout - Writing
     2 - Standard Error - STDERR_FILENO - stderr - Displaying file errors
  */
  pid_t child1; // For command before the pipe symbol.
  pid_t child2; // For command after the pipe symbol.

  // Initiate the pipe and check if pipe was successful.
  if(pipe(fd) < 0)
  {
    printf("Pipe could not be initiated successfully. Please try again later.\n");
    return; // No point in continuing in this function as the pipe was not successfully established, therefore the command won't successfully work. Go back to main.
  }
  
  child1 = fork(); // Create the first child process from the parent process.

  // Similar to the previous function - when executing only one command.
  if(child1 < 0)
  {
    printf("Error in executing command - Child process could not be successfuly created. Please try again later...\n");
    return; // Goes back to main to begin the process anew. Ideal health of the child process for return value should be 0. Then execute the rest.
  }

  if(child1 == 0)
  {
    // For the first child process, we want to write. The results of the first child process act as input for the second child process to parse through.
    close(fd[0]); // Close the reading ability.
    dup2(fd[1], STDOUT_FILENO); // Create a duplicated standard output in place of the file descriptor. 
    close(fd[1]); // Close the file descriptor as well as our pipe should now have access to the system based standard out buffer.

    // Executing the command before the pipe. Results should feed into the pipe and kept stored inside of it.
    if(execvp(userInpParsed[0], userInpParsed) < 0)
    {
      printf("Error in executing command - Command was not successfully sent to the OS. Please try again later...\n"); // In case something went wrong.
      exit(0); // Exit child process.
    }
  }
  else
  {
    child2 = fork(); // Create the second child process to execute commands after the pipe from the parent process.

    if(child2 < 0)
    {
      printf("Error in executing command - Child process could not be successfuly created. Please try again later...\n");
      return; // Goes back to main to begin the process anew. Ideal health of the child process for return value should be 0. Then execute the rest.
    }

    if(child2 == 0)
    {
      // For the second child process, we want to read. This allows the results from the first child process to be read in and processed according to the second command/program.
      close(fd[1]); // Close the writing ability.
      dup2(fd[0], STDIN_FILENO); // Create a duplicated standard input in place of the file descriptor. 
      close(fd[0]); // Close the file descriptor as well as our pipe should now have access to the system based standard input buffer.
      
      if(execvp(userInpParsedPiped[0], userInpParsedPiped) < 0)
      {
	printf("Error in executing command - Command was not successfully sent to the OS. Please try again later...\n"); // In case something went wrong.
	exit(0); // Exit child process.
      }
      
      fflush(stdout); // TEST
    }
    else
    {
      wait(NULL); // Waits for the child process to die if none of the conditions above are met.
    }
  }
  //exit(0); // TEST
}

/*--------------------------------------------------------------------------*/

// Handles redirecting output to a file.
void execCmdRedirect(char** userInpParsed, char** userInpParsedRedirect)
{
  int fd[2]; // File descriptor to respresent 2 states for the pipe. 0 - Read, 1 - Write as per the file descriptor standards.
  /* 0 - Standard Input - STDIN_FILENO - stdin - Reading
     1 - Standard Output - STDOUT_FILENO - stdout - Writing
     2 - Standard Error - STDERR_FILENO - stderr - Displaying file errors
  */
  pid_t child1; // For command before the redirect symbol.
  pid_t child2; // For command after the redirect symbol.

  // Initiate the pipe and check if pipe was successful.
  if(pipe(fd) < 0)
  {
    printf("Pipe could not be initiated successfully. Please try again later.\n");
    return; // No point in continuing in this function as the pipe was not successfully established, therefore the command won't successfully work. Go back to main.
  }
  
  child1 = fork(); // Create the first child process from the parent process.

  // Similar to the previous function - when executing only one command.
  if(child1 < 0)
  {
    printf("Error in executing command - Child process could not be successfuly created. Please try again later...\n");
    return; // Goes back to main to begin the process anew. Ideal health of the child process for return value should be 0. Then execute the rest.
  }

  if(child1 == 0)
  {
    // For the first child process, we want to write. The results of the first child process act as input for the second child process to parse through.
    close(fd[0]); // Close the reading ability.
    dup2(fd[1], STDOUT_FILENO); // Create a duplicated standard output in place of the file descriptor. 
    close(fd[1]); // Close the file descriptor as well as our pipe should now have access to the system based standard out buffer.

    // Executing the command before the pipe. Results should feed into the pipe and kept stored inside of it.
    if(execvp(userInpParsed[0], userInpParsed) < 0)
    {
      printf("Error in executing command - Command was not successfully sent to the OS. Please try again later...\n"); // In case something went wrong.
      exit(0); // Exit child process.
    }
    
    fflush(stdout);
  }
  else
  {
    child2 = fork(); // Create the second child process to execute commands after the pipe from the parent process.

    if(child2 < 0)
    {
      printf("Error in executing command - Child process could not be successfuly created. Please try again later...\n");
      return; // Goes back to main to begin the process anew. Ideal health of the child process for return value should be 0. Then execute the rest.
    }

    if(child2 == 0)
    {
      char write_buf[10000];
      close(fd[1]);

      if(read(fd[0], write_buf, sizeof(write_buf)) != 0)
      {
	printf("\n===================================\nWrite buffer value is: %s\n", write_buf);

	// Need to implement file saving here.
      }

      close(fd[0]);
      fflush(stdout);
      fflush(stdin);
      
      /*
      // For the second child process, we want to read. This allows the results from the first child process to be read in and processed according to the second command/program.
      close(fd[1]); // Close the writing ability.
      dup2(fd[0], STDIN_FILENO); // Create a duplicated standard input in place of the file descriptor. 
      close(fd[0]); // Close the file descriptor as well as our pipe should now have access to the system based standard input buffer.
      
      if(execvp(userInpParsedPiped[0], userInpParsedPiped) < 0)
      {
	printf("Error in executing command - Command was not successfully sent to the OS. Please try again later...\n"); // In case something went wrong.
	exit(0); // Exit child process.
      }
      
      fflush(stdout); // TEST*/
    }
    else
    {
      wait(NULL); // Waits for the child process to die if none of the conditions above are met.
    }
  }
}

/*--------------------------------------------------------------------------*/

int main(void)
{
  char userInput[1024]; // Stores the user input in an array somewhere in memory.
  char* userInpParsed[200]; // Stores the memory addresses in a pointer array to where the user input gets parsed to.
  char* userInpParsedPiped[200]; // Stores the memory addresses in a pointer array to where the user input gets parsed to AFTER the pipe '|' character.
  char* userInpParsedRedirect[200]; // Stores the memory addresses in a pointer array to where the user input gets parsed to AFTER the redirect '>' or '>>' character.
  int typeOfExecFlag = 0; // 0 - Built-in commands, 1 - Normal command execution, 2 - Piped commands execution.

  // We initiate a continous loop to ensure program keeps on running until otherwise told to stop.
  while(1)
  {
    if(userInpTake(userInput) == 1)
    {
      continue; // If it's 1, there was no input from the user. So we need to start at the beginning of the loop again to get another user input.
    }

    typeOfExecFlag = execFlagProcess(userInput, userInpParsed, userInpParsedPiped, userInpParsedRedirect); // This should get the adequate value back to determine what type of execution we want to do. Should it be a normal command execution or piped?
    
    if(typeOfExecFlag == 1)
    {
      // Normal command execution:
      execCmd(userInpParsed);
    }
    else if(typeOfExecFlag == 2)
    {
      // Piped command execution:
      execCmdPiped(userInpParsed, userInpParsedPiped);
    }
    else if(typeOfExecFlag == 3)
    {
      // Redirect command execution:
      execCmdRedirect(userInpParsed, userInpParsedRedirect);
    }
  }
  return 0;
}
