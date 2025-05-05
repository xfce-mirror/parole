foreach size : [16, 24, 32, 48, 64, 96, 128]
  install_data(
    '@0@x@0@'.format(size) / 'org.xfce.parole.png',
    install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / '@0@x@0@'.format(size) / 'apps',
  )
endforeach

foreach size : [16, 48]
  install_data(
    '@0@x@0@'.format(size) / 'parole-extension.png',
    install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / '@0@x@0@'.format(size) / 'apps',
  )
endforeach

install_data(
  '16x16' / 'parole-extension-symbolic.svg',
  install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / '16x16' / 'apps',
)

install_data(
  'scalable' / 'org.xfce.parole.svg',
  install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps',
)
