{
  'targets': [
    {
      'target_name': 'sipster',
      'sources': [
        'src/binding.cc'
      ],
      'conditions': [
        [ 'OS!="win"', {
          'cflags_cc': [
            '<!@(pkg-config --atleast-version=2.2.1 libpjproject)',
            '<!@(pkg-config --cflags libpjproject)',
            '-fexceptions',
            '-Wno-maybe-uninitialized',
          ],
          'libraries': [
            '<!@(pkg-config --libs libpjproject)',
          ],
        }],
      ],
    },
  ],
}