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

## Feature 3 – Column Display (Down then Across)

### Question 1: How does the program know how many columns to display?
It uses the `ioctl()` system call with `TIOCGWINSZ` to read the current terminal width (number of columns).  
The code then divides this width by the maximum filename length plus some spacing to calculate how many columns can fit.

### Question 2: Why “down then across” instead of “across then down”?
The real `ls` arranges items *down the first column*, then moves to the next column to keep items grouped logically.  
It’s easier to scan visually and matches the behavior of most Unix `ls` implementations.

### Question 3: What is the role of `printf("%-*s")` in formatting?
`%-*s` left-aligns the text and pads it with spaces up to the given width, ensuring that all filenames line up in neat columns.

