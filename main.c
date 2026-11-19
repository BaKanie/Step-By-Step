#include <stdio.h>

int main(void)
{
    char title[100];
    printf("Enter your TO DO LIST: ");
    if (fgets(title,sizeof(title), stdin) == NULL) {
        return 1;
    
    }
    
    printf("Ready to do: %s", title);

    return 0;
}