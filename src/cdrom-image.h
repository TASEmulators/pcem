#ifndef CDROM_ISO_H
#define CDROM_ISO_H

#include <stdint.h>

extern char image_path[1024];
extern char image_list[4096];

#ifdef __cplusplus
extern "C" {
#endif

int image_open(char *fn);
void image_next();
void image_previous();
void image_init_list();
void image_reset();
void image_close();

void image_audio_callback(int16_t *output, int len);
void image_audio_stop();

#ifdef __cplusplus
}
#endif

#endif /* ! CDROM_ISO_H */
