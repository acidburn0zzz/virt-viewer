dnl Bash completion support
dnl
dnl Copyright (C) 2017 Red Hat, Inc.
dnl
dnl This library is free software; you can redistribute it and/or
dnl modify it under the terms of the GNU Lesser General Public
dnl License as published by the Free Software Foundation; either
dnl version 2.1 of the License, or (at your option) any later version.
dnl
dnl This library is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl Lesser General Public License for more details.
dnl
dnl You should have received a copy of the GNU Lesser General Public
dnl License along with this library.  If not, see
dnl <http://www.gnu.org/licenses/>.
dnl
dnl Inspired by libvirt code.
dnl

AC_DEFUN([VIRT_VIEWER_ARG_BASH_COMPLETION],[
  m4_divert_text([DEFAULTS], [[enable_bash_completion=check]])
  AC_ARG_ENABLE([bash_completion],
                [AS_HELP_STRING([--enable-bash-completion],
                                [bash completion @<:@default=check@:>@])])
  m4_divert_text([DEFAULTS], [[with_bash_completions_dir=check]])
  AC_ARG_WITH([bash_completions_dir],
              [AS_HELP_STRING([--with-bash-completions-dir=DIR],
                              [directory containing bash completions scripts @<:@default=check@:>@])])
])


AC_DEFUN([VIRT_VIEWER_CHECK_BASH_COMPLETION], [
  if test "x$enable_bash_completion" != "xno" ; then
    PKG_CHECK_MODULES(BASH_COMPLETION, bash-completion >= $BASH_COMPLETION_REQUIRED,
                      [with_bash_completion=yes],
                      [with_bash_completion=no])
    if test "x$enable_bash_completion:x$with_bash_completion" = "yes:no"; then
      m4_default(fail_action,
        [AC_MSG_ERROR([You must install the ]pc_name[ >= ]pc_version[ pkg-config module to compile virt-viewer])])
    fi
    enable_bash_completion=$with_bash_completion
  fi

  if test "x$with_bash_completion" = "xyes" ; then
    if test "x$with_bash_completions_dir" = "xcheck"; then
      AC_MSG_CHECKING([for bash-completions directory])
      BASH_COMPLETIONS_DIR="$($PKG_CONFIG --variable=completionsdir bash-completion)"
      AC_MSG_RESULT([$BASH_COMPLETIONS_DIR])
      with_bash_completions_dir=$BASH_COMPLETIONS_DIR

      dnl Replace bash completions's exec_prefix with our own.
      dnl Note that ${exec_prefix} is kept verbatim at this point in time,
      dnl and will only be expanded later, when make is called: this makes
      dnl it possible to override such prefix at compilation or installation
      dnl time
      bash_completions_prefix="$($PKG_CONFIG --variable=prefix bash-completion)"
      if test "x$bash_completions_prefix" = "x" ; then
        bash_completions_prefix="/usr"
      fi

      BASH_COMPLETIONS_DIR='${exec_prefix}'"${BASH_COMPLETIONS_DIR#$bash_completions_prefix}"
    elif test "x$with_bash_completions_dir" = "xno" || test "x$with_bash_completions_dir" = "xyes"; then
      AC_MSG_ERROR([bash-completions-dir must be used only with valid path])
    else
      BASH_COMPLETIONS_DIR=$with_bash_completions_dir
    fi
    AC_SUBST([BASH_COMPLETIONS_DIR])
  fi

  AM_CONDITIONAL([WITH_BASH_COMPLETION], [test "x$enable_bash_completion" = "xyes"])
])


AC_DEFUN([VIRT_VIEWER_RESULT_BASH_COMPLETION],[
  if test "x$enable_bash_completion" = "xyes" ; then
    AC_MSG_NOTICE([bash completion: $enable_bash_completion DIR: [$with_bash_completions_dir]])
  else
    AC_MSG_NOTICE([bash completion: $enable_bash_completion])
  fi
  AC_MSG_NOTICE([])
])
