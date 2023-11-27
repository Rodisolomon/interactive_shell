# interactive_shell

This is my computer system class final project. 

Refer to this link: https://people.cs.uchicago.edu/~castuardo/cs-154/p4/p4shell.html

Design:

1. Core Shell Loop:
The shell operates in an interactive loop that continually prompts the user for input. Each iteration of the loop represents a lifecycle of processing a command.
The prompt used is "myshell> ", aligning with traditional Unix shell standards.

2. Command Parsing: Input lines are parsed to identify the command and its arguments. This involves handling a maximum command length of 512 bytes, excluding the newline character.
The parsing mechanism also deals with white spaces and special characters like ";" and ">" to differentiate between commands, arguments, and redirection operators.

3. Process Handling:
For each command, a new process is created using fork(), except for built-in commands which are executed within the shell process.
The execvp() system call is used to execute commands in the child process.
The parent process waits for the completion of the child process using wait() or waitpid() before resuming the next iteration of the loop.

4. Built-in Command Implementation:
The shell directly executes built-in commands like exit, cd, and pwd without creating a new process.
These are implemented using system/library calls such as exit(0), chdir, getcwd, and getenv.

5. Handling Multiple Commands:
Multiple commands in a single input line are supported, separated by ";". The shell executes these commands sequentially.

6. Redirection Features:
Standard output redirection is implemented using the ">" character, enabling users to direct the output of commands to files instead of the screen.
Custom advanced redirection ">+" is introduced, allowing output insertion at the beginning of a file, a unique feature not typically found in standard shells.

7. Defensive Programming:
The shell includes comprehensive error handling to manage invalid inputs, such as excessively long command lines or nonexistent commands.
A consistent error message is displayed for various error scenarios, enhancing user experience and debuggability.

8. Batch Mode Support:
In addition to interactive mode, the shell supports a batch mode where commands are read from a specified file. This mode is essential for efficient testing and automation.
The shell reads each command line from the batch file and executes them as if they were entered interactively.

9. User Interface and Experience:
The shell interface is user-friendly, with clear prompts and error messages.
It is designed to mimic the behavior of traditional Unix shells, making it intuitive for users familiar with Unix or Linux environments.

