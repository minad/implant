#ifndef COLOR_H
#define COLOR_H

#define FG_BLACK     _color("\033[0;30m")
#define FG_RED       _color("\033[0;31m")
#define FG_GREEN     _color("\033[0;32m")
#define FG_YELLOW    _color("\033[0;33m")
#define FG_BLUE      _color("\033[0;34m")
#define FG_MAGENTA   _color("\033[0;35m")
#define FG_CYAN      _color("\033[0;36m")
#define FG_WHITE     _color("\033[0;37m")

#define FG_LTBLACK   _color("\033[1;30m")
#define FG_LTRED     _color("\033[1;31m")
#define FG_LTGREEN   _color("\033[1;32m")
#define FG_LTYELLOW  _color("\033[1;33m")
#define FG_LTBLUE    _color("\033[1;34m")
#define FG_LTMAGENTA _color("\033[1;35m")
#define FG_LTCYAN    _color("\033[1;36m")
#define FG_LTWHITE   _color("\033[1;37m")

#define BG_BLACK     _color("\033[0;40m")
#define BG_RED       _color("\033[0;41m")
#define BG_GREEN     _color("\033[0;42m")
#define BG_YELLOW    _color("\033[0;43m")
#define BG_BLUE      _color("\033[0;44m")
#define BG_MAGENTA   _color("\033[0;45m")
#define BG_CYAN      _color("\033[0;46m")
#define BG_WHITE     _color("\033[0;47m")

#define FG_BD        _color("\033[1m")
#define FG_UL        _color("\033[4m")
#define NOCOLOR      _color("\033[0m")

extern bool _colorEnabled;
inline const char* _color(const char* c) {
	if (_colorEnabled)
		return c;
	return "";
}

#endif
