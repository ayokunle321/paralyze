#include <stdio.h>
#include <string.h>
#include <ctype.h>

void process_text() {
    char text[1000] = "This is a sample text for processing";
    char processed[1000];
    int len = strlen(text);
    
    // Character transformation - should be safe
    for (int i = 0; i < len; i++) {
        processed[i] = toupper(text[i]);
    }
    
    // Character counting - unsafe
    int vowels = 0;
    for (int i = 0; i < len; i++) {
        char c = tolower(text[i]);
        if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u') {
            vowels++;  // Reduction pattern
        }
    }
}

int main() {
    process_text();
    return 0;
}