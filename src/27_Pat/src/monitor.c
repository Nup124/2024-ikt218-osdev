#include "monitor.h"
#include "common.h"
#include "libc/stdarg.h"
#include "libc/stdint.h"

u16int cursor_y = 0;
u16int cursor_x = 0;
u16int *video_memory = (u16int *)0xB8000;
u16int cursorLocation = 0; // Declare cursorLocation

// Function to calculate the length of a null-terminated string
size_t strlen(const char *str) {
    size_t length = 0;
    while (*str != '\0') {
        ++length;
        ++str;
    }
    return length;
}

// Function to calculate the absolute value of an integer
int abs(int x) {
    return x < 0 ? -x : x;
}

// Updates the hardware cursor.
static void move_cursor()
{
    cursorLocation = cursor_y * 80 + cursor_x;
    outb(0x3D4, 14);                  // Tell the VGA board we are setting the high cursor byte.
    outb(0x3D5, cursorLocation >> 8); // Send the high cursor byte.
    outb(0x3D4, 15);                  // Tell the VGA board we are setting the low cursor byte.
    outb(0x3D5, cursorLocation);      // Send the low cursor byte.
}

// Scrolls the text on the screen up by one line.
static void scroll()
{
    // Get a space character with the default colour attributes.
    u8int attributeByte = (0 /*black*/ << 4) | (15 /*white*/ & 0x0F);
    u16int blank = 0x20 /* space */ | (attributeByte << 8);

    // Row 25 is the end, this means we need to scroll up
    if(cursor_y >= 25)
    {
        // Move the current text chunk that makes up the screen
        // back in the buffer by a line
        int i;
        for (i = 0*80; i < 24*80; i++)
        {
            video_memory[i] = video_memory[i+80];
        }

        // The last line should now be blank. Do this by writing
        // 80 spaces to it.
        for (i = 24*80; i < 25*80; i++)
        {
            video_memory[i] = blank;
        }
        // The cursor should now be on the last line.
        cursor_y = 24;
    }
}

// Writes a single character out to the screen.
void monitor_put(char c)
{
    // The background colour is black (0), the foreground is white (15).
    u8int backColour = 0;
    u8int foreColour = 15;

    // The attribute byte is made up of two nibbles - the lower being the
    // foreground colour, and the upper the background colour.
    u8int  attributeByte = (backColour << 4) | (foreColour & 0x0F);
    // The attribute byte is the top 8 bits of the word we have to send to the
    // VGA board.
    u16int attribute = attributeByte << 8;
    u16int *location;

    // Handle a backspace, by moving the cursor back one space
    if (c == 0x08 && cursor_x)
    {
        cursor_x--;
    }

    // Handle a tab by increasing the cursor's X, but only to a point
    // where it is divisible by 8.
    else if (c == 0x09)
    {
        cursor_x = (cursor_x+8) & ~(8-1);
    }

    // Handle carriage return
    else if (c == '\r')
    {
        cursor_x = 0;
    }

    // Handle newline by moving cursor back to left and increasing the row
    else if (c == '\n')
    {
        cursor_x = 0;
        cursor_y++;
    }
    // Handle any other printable character.
    else if(c >= ' ')
    {
        location = video_memory + (cursor_y*80 + cursor_x);
        *location = c | attribute;
        cursor_x++;
    }

    // Check if we need to insert a new line because we have reached the end
    // of the screen.
    if (cursor_x >= 80)
    {
        cursor_x = 0;
        cursor_y ++;
    }

    // Scroll the screen if needed.
    scroll();
    // Move the hardware cursor.
    move_cursor();
}

// Clears the screen, by copying lots of spaces to the framebuffer.
void monitor_clear()
{
    // Make an attribute byte for the default colours
    u8int attributeByte = (0 /*black*/ << 4) | (15 /*white*/ & 0x0F);
    u16int blank = 0x20 /* space */ | (attributeByte << 8);

    int i;
    for (i = 0; i < 80*25; i++)
    {
        video_memory[i] = blank;
    }

    // Move the hardware cursor back to the start.
    cursor_x = 0;
    cursor_y = 0;
    move_cursor();
}

// Function to convert an integer to a string
void int_to_string(int value, char *buffer, int base)
{
    if (value == 0)
    {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    int i = 0;
    // Check if the value is negative
    if (value < 0) {
        buffer[i++] = '-';
        value = -value;
    }
    
    int start = i; // Store the starting index of the number
    
    while (value != 0)
    {
        int rem = value % base;
        buffer[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        value = value / base;
    }
    buffer[i] = '\0';

    // Reverse the string
    int len = i;
    for (i = start; i < len / 2; ++i)
    {
        char temp = buffer[i];
        buffer[i] = buffer[len - i - 1];
        buffer[len - i - 1] = temp;
    }
}

// Function to convert a pointer address to a string
void pointer_to_string(void *ptr, char *buffer)
{
    // Assuming sizeof(void*) == sizeof(unsigned long) for simplicity
    unsigned long num = (unsigned long)ptr;
    int_to_string(num, buffer, 16);
}

// Function to convert a fixed-point decimal number to a string
void fixed_point_to_string(int value, char *buffer)
{
    // Integer part
    int_to_string(value / 100, buffer, 10);
    int len = strlen(buffer);

    // Decimal point
    buffer[len] = '.';
    buffer[len + 1] = '\0';

    // Decimal part
    int_to_string(abs(value % 100), buffer + len + 1, 10);

    // Add leading zero if necessary
    if (abs(value % 100) < 10) {
        buffer[len + 2] = '0';
        buffer[len + 3] = '\0';
    }
}

// Printf function implementation
void printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    // Iterate over the format string
    for (const char *p = format; *p != '\0'; ++p)
    {
        // If the character is not a format specifier, print it directly
        if (*p != '%')
        {
            monitor_put(*p);
            continue;
        }
        // Move to the next character after '%'
        ++p;

        // Check the type of format specifier
        switch (*p)
        {
            case 'd':
            {
                // Extract the next argument as an integer and print it
                int num = va_arg(args, int);
                char buffer[20];
                int_to_string(num, buffer, 10);
                for (int i = 0; buffer[i] != '\0'; ++i)
                {
                    monitor_put(buffer[i]);
                }
                break;
            }
            case 'x':
            {
                // Extract the next argument as an unsigned int and print it in hexadecimal format
                unsigned int num = va_arg(args, unsigned int);
                char buffer[20];
                int_to_string(num, buffer, 16);
                for (int i = 0; buffer[i] != '\0'; ++i)
                {
                    monitor_put(buffer[i]);
                }
                break;
            }
            case 'p':
            {
                // Extract the next argument as a pointer and print its address
                void *ptr = va_arg(args, void*);
                char buffer[20];
                pointer_to_string(ptr, buffer);
                monitor_put('0');
                monitor_put('x');
                for (int i = 0; buffer[i] != '\0'; ++i)
                {
                    monitor_put(buffer[i]);
                }
                break;
            }
            case 'f':
            {
                // Extract the next argument as a floating-point number (assumed as fixed-point for simplicity) and print it
                float num = (float)va_arg(args, double); // Assuming float is converted to double in va_arg
                int value = (int)(num * 100); // Convert to fixed-point with two decimal places
                char buffer[20];
                fixed_point_to_string(value, buffer);
                for (int i = 0; buffer[i] != '\0'; ++i)
                {
                    monitor_put(buffer[i]);
                }
                break;
            }
            case 'c':
            {
                // Extract the next argument as a single character and print it
                char c = (char)va_arg(args, int);
                monitor_put(c);
                break;
            }
            default:
                // If the format specifier is not recognized, print it as is
                monitor_put('%');
                monitor_put(*p);
                break;
        }
    }
    va_end(args);
}
