To compile just type make in the enclosing folder and run ./smallsh in the terminal. 

The small shell is pared down version of a typical CLI UNIX shell. The small shell provides simple commands such as cd, status and exit. Small shell will also run your favorite shell commands provided in you system PATH variable.  

Commands must be presented in the following format.
command [arg1 arg2 ...] [< input_file] [> output_file] [&]

There must be spaces between each command, argument, filename , <,  > and &.   

Built in commands:
cd [path to change to]: Changes working directory to directory passed as an argument
status [no arguments]: Prints status of last exited command 
exit [no arguments]: Close small shell and return to your shell 

Commands can be run as background processes by ending the command with & character. Alternatively sending the SIGSTP signal with CTRL+Z will toggle the foreground only mode, to exit foreground only mode send CTRL+Z again.  
