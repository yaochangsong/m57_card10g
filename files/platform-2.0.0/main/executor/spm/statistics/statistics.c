
spm_statistics_t *spm_stats;



void statistics_init(void)
{
    spm_statistics_t *s = NULL;
    s = calloc(1, sizeof(*s));
    if(!s)
        return;

    spm_stats = s;
}
