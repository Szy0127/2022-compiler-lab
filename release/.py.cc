#include <stdio.h>
struct string { 
        int length;         
        unsigned char chars[1]; 
    };                              
    void print_string(struct string *s) {   
        int i;                               
        unsigned char *p = s->chars;            
        for (i = 0; i < s->length; i++, p++) putchar(*p);  
    }
extern "C" void print_2(long a0){
	printf("%ld",a0);
	printf("\n");
}
