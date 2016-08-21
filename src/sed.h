/*
 * sed.h: a very basic implementation of the unix sed command
 */

#ifndef SED_H
#define SED_H

/* sed: perform a basic sed substitution command on input */
int sed(char *s, size_t max, const char *input, const char *sedcmd);

#endif /* SED_H */
