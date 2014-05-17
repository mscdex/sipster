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
        [ 'OS=="mac"', {
          'xcode_settings': {
            'OTHER_CFLAGS': [
              '-fexceptions',
              '-frtti',
            ],
          },

          # begin gyp stupidity workaround =====================================
          'ldflags!': [
            '-framework CoreAudio',
          ],
          'libraries!': [
            'CoreServices', 
            'AudioUnit',
            'AudioToolbox',
            'Foundation',
            'AppKit',
            'QTKit',
            'QuartzCore',
            'OpenGL',
          ],
          'libraries': [
            'CoreAudio.framework',
            'CoreServices.framework',
            'AudioUnit.framework',
            'AudioToolbox.framework',
            'Foundation.framework',
            'AppKit.framework',
            'QTKit.framework',
            'QuartzCore.framework',
            'OpenGL.framework',
          ],
          # end gyp stupidity workaround =======================================

        }],
      ],
    },
  ],
}
