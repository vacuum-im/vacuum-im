#ifndef DEF_STANZAHANDLERORDERS_H
#define DEF_STANZAHANDLERORDERS_H

//Message In
#define SHO_MI_REMOTECONTROL          100
#define SHO_MI_PRIVATESTORAGE         300
#define SHO_MI_CAPTCHAFORMS           300
#define SHO_MI_MULTIUSERCHAT_INVITE   300
#define SHO_MI_CHATSTATES             400
#define SHO_MI_MULTIUSERCHAT          500

//Message Out
#define SHO_MO_ARCHIVER               200
#define SHO_MO_CHATSTATES             500

//Presence In
#define SHO_PI_AVATARS                400
#define SHO_PI_SERVICEDISCOVERY       800
#define SHO_PI_MULTIUSERCHAT          900

//Iq Message In
#define SHO_IMI_ROSTEREXCHANGE        100

//Iq Message Presence In
#define SHO_IMPI_BITSOFBINARY         100

//Iq Message Presence Out
#define SHO_IMPO_CAPTCHAFORMS         100

//Default
#define SHO_DEFAULT                   1000

#endif // DEF_STANZAHANDLERORDERS_H
