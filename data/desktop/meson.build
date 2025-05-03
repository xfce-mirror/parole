mimelist = meson.project_source_root() / 'data' / 'mime' / 'mime-type-list.txt'
desktop_sh = find_program('desktop.sh')

desktop_in = configure_file(
  input: 'org.xfce.Parole.desktop.in.in',
  output: 'org.xfce.Parole.desktop.in',
  command: [desktop_sh, '@INPUT@', mimelist, '@OUTPUT@'],
)

i18n.merge_file(
  input: desktop_in,
  output: 'org.xfce.Parole.desktop',
  po_dir: '..' / '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / 'applications',
)
