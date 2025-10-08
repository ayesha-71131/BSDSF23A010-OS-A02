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

## Feature 4 – Horizontal Display (-x) and Argument Parsing

### What was added
This feature introduced command-line option parsing for `-l` (long listing) and `-x` (horizontal display).  
- `getopt()` was used to handle both flags.  
- If `-l` is given, detailed file information is printed.  
- If `-x` is given, files are displayed across rows (across-then-down).  
- If no flag is given, the program defaults to down-then-across column display.

### Key Concepts
- **Option parsing** lets us add flexible behavior without changing code logic.  
- **Precedence rule:** `-l` overrides `-x`, just like in real `ls`.  
- The horizontal display rearranges the loop order: index = row * cols + col.

## Feature 5 – Alphabetical Sorting (v1.4.0)

### What was added
This feature adds alphabetical sorting (A–Z) for all display modes using the C library function `qsort()`.  
After reading directory entries, the list of file names is sorted before printing.

### How it works
`qsort()` rearranges array elements based on the result of a comparison function.  
We implemented:
```c
int compare_names(const void *a, const void *b) {
    return strcasecmp(*(char **)a, *(char **)b);
}

## Feature 6 – Recursive Listing (-R) (v1.5.0)

### What was added
This feature allows listing subdirectories recursively when the `-R` option is used.  
Each directory name is printed before its contents, and the program calls itself for every subdirectory found inside.

### How it works
1. When the `-R` flag is detected by `getopt()`, the program switches to recursive mode.  
2. After printing the files of the current directory, it loops through all entries.  
3. For each entry that is a directory (and not `.` or `..`), it prints the path and calls the listing function again on that path.

### Key concepts
- **Recursion:** A function calling itself for subdirectories.
- **Path building:** Each nested directory path is built using `snprintf(path, "%s/%s", current, entry)`.
- **Base condition:** Stops recursion when no more subdirectories exist.

---

## Feature 7 – Combined Options (-l, -x, -R) (v1.6.0)

### What was added
This version allows using options together, for example:
- `ls -lR` → Long listing, recursive  
- `ls -xR` → Horizontal column display, recursive  
- `ls -lxR` → Combination of all supported options  

The program parses all flags in a single pass and adjusts its behavior accordingly.

### How it works
1. Multiple flags are handled through `getopt()` (e.g. `while ((opt = getopt(argc, argv, "lxR")) != -1)`).
2. The recursive function now receives parameters telling it which display mode to use.
3. For each directory, it applies the right printing function before descending into subdirectories.

### Key concepts
- **Flag combination:** Bitwise or logical tracking of selected options.  
- **Function reuse:** The same `do_long()`, `do_horizontal()`, or default printer is reused recursively.  
- **Clean recursion:** Keeps directory structure clear and avoids infinite loops by skipping `.` and `..`.

---

### Summary of Features 6 & 7
- Added recursive directory traversal (`-R`).
- Supported combining `-l`, `-x`, and `-R` for flexible output.
- All features maintain alphabetical sorting and clean, readable output.
