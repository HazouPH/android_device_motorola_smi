#include <string.h>
#include <unistd.h>
#include <sound/asound.h>
#include <tinyalsa/asoundlib.h>

struct mixer_ctl {
    struct mixer *mixer;
    struct snd_ctl_elem_info *info;
    char **ename;
};

struct mixer {
    int fd;
    struct snd_ctl_card_info card_info;
    struct snd_ctl_elem_info *elem_info;
    struct mixer_ctl *ctl;
    unsigned int count;
};

struct mixer_ctl *mixer_get_ctl_by_name(struct mixer *mixer, const char *name)
{
    return mixer_get_ctl_by_name_and_index(mixer, name, 0);
}

struct mixer_ctl *mixer_get_ctl_by_name_and_index(struct mixer *mixer,
                                                  const char *name,
                                                  unsigned int index)
{
    unsigned int n;

    if (!mixer)
        return NULL;

    for (n = 0; n < mixer->count; n++)
        if (!strcmp(name, (char*) mixer->elem_info[n].id.name))
            if (index-- == 0)
                return mixer->ctl + n;

    return NULL;
}
