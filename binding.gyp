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
            '<!@(pkg-config --atleast-version=2.2.0 libpjproject)',
            '<!@(pkg-config --cflags libpjproject)',
            '-fexceptions',
          ],
          'libraries': [
            '<!@(pkg-config --libs libpjproject)',
          ],
        }],
      ],
    },
  ],
}