#ifndef scopedlogger_h
#define scopedlogger_h

/*it's expensive, disable it by default*/
#define DISABLE_SCOPED_LOGGER
#ifndef DISABLE_SCOPED_LOGGER
#include "log.h"

class ScopedLogger {
  public:
    ScopedLogger(const char *str)
    {
        m_str = str;
        INFO("+%s", m_str);
    }
    ~ScopedLogger()
    {
        INFO("-%s", m_str);
    }

  private:
    const char *m_str;
};

#define FUNC_ENTER() ScopedLogger __func_loggger__(__func__)
#else
#define FUNC_ENTER()
#endif

#endif  //scopedlogger_h
