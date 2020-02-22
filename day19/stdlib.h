int _strcmp(char *s1, char *s2){
    while(*s1 != '\0' && *s2 != '\0'){
        if(*s1 != *s2) {
            break;
        }
        s1++;
        s2++;
    }

    return(*s1 - *s2);
}

int _strncmp(char *s1, char *s2, int n){
    int i = 0;
    while(*s1 != '\0' && *s2 != '\0'){
        if(*s1 != *s2) {
            break;
        }
        s1++;
        s2++;
        i++;
        if (i = n) {
            return 0;
        }
    }

    return(*s1 - *s2);
}

int _strlen(char *str){
    int i;
    for(i = 0; str[i] != '\0'; i++);
    return(i);
}
