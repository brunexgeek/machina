#ifndef MACHINA_ERRORS_H
#define MACHINA_ERRORS_H


#ifdef __arm__

#define EOK                            (0x00)
#define EREBOOT                        (0x01)

#define ERESTART                       (0x02)
#define ESHUTDOWN                      (0x03)

#define EINVALID                       (0x04)
#define EEXHAUSTED                     (0x05)

#endif

#endif // MACHINA_ERRORS_H