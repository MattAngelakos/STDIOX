#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>

/*This function checks if files are already opened using the filename*/
int __is_file_open(char *filename)
{
    int file;                           // temp file descriptor to check if the file is already opened
    struct stat file_stat, i_file_stat; // file stat of temp file and current ith process file
    int table_size = getdtablesize();   // get the tablesize in order to loop through it

    // Open the file (if it exists) to get its inode number
    if (stat(filename, &file_stat) == -1)
    {
        return 0; // file does not exist
    }

    // Check if any open file has the same inode number as the file we want to open
    for (int i = 0; i < table_size; i++)
    {
        if ((file = fcntl(i, F_DUPFD)) != -1)
        {
            if (fstat(file, &i_file_stat) != -1)
            {
                if (file_stat.st_ino == i_file_stat.st_ino)
                {
                    close(file); // if the inode number is the same and made it past all checks then the file is already opened we close the duplicate and return i
                    return i;    // file is open return i to give back proper inode number to use
                }
            }
        }
    }
    return 0; // file is not open
}

/*This function turns an integer to a string*/
void __printInt(int desr, int num)
{
    char buffer[12]; // min size of a integer is 11 characters so we do 12 cause I don't like odd numbers
    int i = 0;
    int sign = 0; // used as a flag to check for negatives
    if (num < 0)
    {
        sign = 1;
        num = -num;
    }
    do
    {
        buffer[i++] = num % 10 + '0'; // the current digit is the modulus 10 of the current num and we make it a char by adding '0'
        num /= 10;                    // divide by 10 to get to the next digit
    } while (num != 0);
    if (sign)
    {
        buffer[i++] = '-'; // add the negative sign on to the last part of the string
    }
    // Reverse the buffer to print the number in the correct order
    for (int j = 0; j < i / 2; j++)
    {
        char temp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = temp;
    }
    // Write the buffer to desr using the write system call
    write(desr, buffer, i);
}

/*Helper function for turning floats into string basically the same as ints but tweeked for floats*/
void __printFloat(int desr, float num, int precision) // we have precision here im hardcoding to 6 for now but idk what he wants it at lol
{
    int i = 0;
    int sign = 0;
    int currDigit;
    int beforeDecimal = (int)num;             // this is the number before the decimal
    float afterDecimal = num - beforeDecimal; // this is the number after the decimal
    char buffer[100];                         // large buffer cause idk the min float
    if (num < 0)                              // same as ints but now change both to negatives
    {
        sign = 1;
        beforeDecimal = -beforeDecimal;
        afterDecimal = -afterDecimal;
    }
    do
    {
        buffer[i++] = beforeDecimal % 10 + '0'; // the current digit is the modulus 10 of the current num and we make it a char by adding '0'
        beforeDecimal /= 10;                    // divide by 10 to get to the next digit
    } while (beforeDecimal != 0);
    if (sign)
    {
        buffer[i++] = '-'; // add the negative sign on to the last part of the string
    }
    for (int j = 0; j < i / 2; j++) // same reverse as before
    {
        char temp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = temp;
    }
    if (precision > 0) // if the percision is 0 no need to add decimals or anything
    {
        buffer[i++] = '.';    // add it
        while (precision > 0) // same concept as the above for turning strings to floats but now in reverse.
        {
            afterDecimal *= 10; // this is used once again to offset where we are in the float but now we dont need to divide as we can treat it as a normal number
            currDigit = (int)afterDecimal;
            buffer[i++] = currDigit + '0';
            afterDecimal -= currDigit; // minus to get the correct digit of the current number
            precision--;               // lower the precision
        }
    }
    // we actually don't need to reverse as since it acts as a mirror of the before the decimal point it is already in the correct order
    //  Write the buffer to desr using the write system call
    write(desr, buffer, i);
}
/*Helper function to condense the printout for fprintfx*/
int __printOut(int file, char format, void *data)
{
    if (format == 's') // if the format is string do this
    {
        int i = 0;
        char curr_char = *((char *)data);
        while (curr_char != '\0')
        {
            if (write(file, &curr_char, 1) != 1)
            {
                errno = EIO;
                return -1;
            }
            i++;
            curr_char = *((char *)data + i);
        }
    }
    else if (format == 'd') // if d then integer
    {
        __printInt(file, *((int *)data)); // use the helper function above to printout the integer
    }
    else if (format == 'f')
    {
        __printFloat(file, *((float *)data), 6);
    }
    else // if not s or d error
    {
        errno = EIO;
        return -1;
    }
    return 0;
}

/*fprintfx new function to basically become fprint*/
int fprintfx(char *filename, char format, void *data)
{
    if (data == NULL) // if there is no where to write to we return error
    {
        errno = EIO;
        // write(STDERR_FILENO, "Error: NULL destination\n", 24);
        return -1;
    }
    if (filename == NULL || strcmp(filename, "") == 0) // if the specification of the filename is empty we assume its stdout
    {
        if (__printOut(STDOUT_FILENO, format, data) != 0) // we attempt to printout our data to stdout if there is an error we return -1 if its good then 1
        {
            return -1;
        }
        write(STDOUT_FILENO, "\n", 1);
        return 0;
    }
    else
    {
        if (access(filename, F_OK) != -1) // we attempt to access the file given if it exists we run this if
        {
            int file = __is_file_open(filename); // check if the file has been opened
            if (file == 0)
            {
                file = open(filename, O_WRONLY | O_APPEND); // we open this file and see we are going to write only and append
            }
            if (file == -1) // if there was an error opening return error
            {
                errno = EIO;
                // write(STDERR_FILENO, "Failed to open file.\n", 21);
                return -1;
            }
            write(file, "\n", 1);
            if (__printOut(file, format, data) != 0) // we attempt to printout to this file same as above
            {
                return -1;
            }
            return 0;
        }
        else
        {
            mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP;
            mode &= ~(S_IXUSR | S_IWGRP | S_IXGRP | S_IXOTH | S_IWOTH | S_IROTH); // specify perms for the new file
            int file = open(filename, O_CREAT | O_WRONLY, mode); // we create our new file using these permissions
            if (file == -1) // same error as above
            {
                errno = EIO;
                return -1;
            }
            chmod(filename, mode);                   // to make sure we have the correct perms we use chmod
            if (__printOut(file, format, data) != 0) // same printout and error as above
            {
                return -1;
            }
            return 0;
        }
    }
}

/* Helper function in order to simplify fscanfx*/
int __do_scan(int read_output, int file, char *buffer, size_t bytes, size_t size, char format, void *dst)
{
    int end = 0;
    size_t newsize = size;
    // We read 1 byte from the file, we will keep reading until we either reach the EOF or a newline
    while ((read_output = read(file, buffer + bytes, 1)) > 0)
    {
        end = 1;
        bytes += read_output;          // bytes is the total bytes read and is used to keep track of location
        if (buffer[bytes - 1] == '\n') // again if newline then break as 1 line has been read
        {
            break;
        }
        if (bytes == size) // if our total bytes is now equal to the size of the buffer we reallocate it now 128 more
        {
            newsize += size;
            buffer = realloc(buffer, newsize);
        }
    }
    if (read_output == 0 && end == 0) // for EOF returns 1 as 0ot an error like -1 but we don't want to return 0 for boolean purposes
    {
        free(buffer);
        return 1;
    }
    if (read_output < 0) // for errors reading from the file
    {
        errno = EIO;
        free(buffer);
        // write(STDERR_FILENO, "Error reading from file.\n", 25);
        return -1;
    }
    if (format == 's') // if the format is a string we wipe all previous memory from the string we wish to write to then copy the buffer into it
    {
        int x = strlen((char *)dst);
        memset(dst, 0, x);
        if (read_output == 0)
        {
            memcpy(dst, buffer, bytes);
        }
        else
        {
            memcpy(dst, buffer, bytes - 1);
        }
    }
    else if (format == 'd') // if the format is an integer we convert the buffer string into an integer and set our data pointer to point to it
    {
        *((int *)dst) = atoi(buffer);
    }
    else if (format == 'f')
    {
        *((float *)dst) = atof(buffer);
    }
    else
    {
        errno = EIO;
        free(buffer);
        return -1;
    }
    free(buffer);
    return 0; // return 0 if success
}

int fscanfx(char *filename, char format, void *dst)
{
    if (dst != NULL) // if the data isnt null we can actually run this function
    {
        size_t size = 128;                                 // set our inital buffer to 128
        size_t bytes = 0;                                  // set the amount of bytes read so far to 0
        int read_output = 0;                               // read output same thing
        char *buffer = (char *)malloc(size);               // we create the buffer
        if (filename == NULL || strcmp(filename, "") == 0) // if either of these are met it indicates scanning from keyboard and we do this
        {
            int file = STDIN_FILENO; // open the keyboard
            if (file < 0)            // if there was an error opening it return error
            {
                errno = EIO;
                free(buffer);
                return -1;
            }
            return __do_scan(read_output, file, buffer, bytes, size, format, dst);
        }
        else
        {
            if (access(filename, F_OK) != -1) // if file exists do this
            {
                int file = __is_file_open(filename); // check if the file has been opened
                if (file != 0)                       // with the correct inode number we now attempt to read a line from the file
                {
                    return __do_scan(read_output, file, buffer, bytes, size, format, dst);
                }
                else
                {
                    file = open(filename, O_RDONLY); // otherwise we open and do the same thing
                    return __do_scan(read_output, file, buffer, bytes, size, format, dst);
                }
            }
            else // if the file doesnt exist we return an error
            {
                errno = ENOENT;
                // write(STDERR_FILENO, "File does not exist.\n", 21);
                free(buffer);
                return -1;
            }
        }
    }
    return 0;
}

int clean() // Just go through the whole table of opened processes. If its not 0, 1, 2 for stdout, stdin, stderr then we close it
{
    int table_size = getdtablesize();
    for (int i = 3; i < table_size; i++)
    {
        if (fcntl(i, F_GETFD) != -1)
        { // check if file descriptor is open
            if (close(i) == -1 && i >= 0)
            {
                // Log the error but continue closing other file descriptors
                errno = EIO;
                return -1;
            }
        }
    }
    return 0;
}