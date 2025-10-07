## Feature 2 – Long Listing Format (-l)

### Question 1: What is the crucial difference between the stat() and lstat() system calls?
The `stat()` system call follows symbolic links and gives information about the target file.  
The `lstat()` system call gives information about the link itself.  
In the context of the ls command, `lstat()` is more appropriate when we want to display details of a symbolic link without following it.

### Question 2: How can we use bitwise operators and predefined macros to extract information from st_mode?
The `st_mode` field in the `struct stat` contains both file type and permission bits.  
We use bitwise AND (`&`) with macros like `S_IFDIR`, `S_IRUSR`, `S_IWUSR`, etc., to test specific bits.

Example:
```c
if (S_ISDIR(st.st_mode)) printf("This is a directory\n");
if (st.st_mode & S_IRUSR) printf("User has read permission\n");
