import os
import fnmatch
import platform

Decider('MD5')

env =Environment()

env['AR'] = 'arm-none-eabi-ar'
env['AS'] = 'arm-none-eabi-as'
env['CC'] = 'arm-none-eabi-gcc'
env['CXX'] = 'arm-none-eabi-g++'
env['LINKER'] = 'arm-none-eabi-gcc'
env['OBJCOPY'] = 'arm-none-eabi-objcopy'
env['PROGSUFFIX'] = '.elf'

Objcopy = Builder(action = env['OBJCOPY'] + ' -O $TYPE $SOURCE $TARGET') 
env.Append(BUILDERS = {'Objcopy':Objcopy})

def all_files(dir, ext='.c',level=6):
  files = []
  for i in range(1, level):
    files += Glob(dir + ('/*' * i)+ext) 
  return files


INCLUDE_PATH = [
  '#inc',
  '#lib/STM32F2xx_StdPeriph_Driver/inc',
  '#lib/CMSIS/inc',
  '#boot_driver/inc'
  ]

LIB_PATH = [
  'lib'
  ]

CFLAGS = [
  '-DSTM32F2XX',
  '-DUSE_STDPERIPH_DRIVER',
  '-DDEBUG',
  '-Os',
  #'-Wall',
  #'-Wextra',
  #'-Wimplicit-function-declaration',
  #'-Wredundant-decls',
  #'-Wmissing-prototypes',
  #'-Wstrict-prototypes',
  #'-Wundef',
  #'-Wshadow',
  #'-fno-common',
  '-mcpu=cortex-m3',
  '-mthumb',
  #'-Wstrict-prototypes',
  '-ffunction-sections',
  '-fdata-sections',
  '-MD',
  #'-ggdb3',
 ]

LDFILE = 'STM32F215ZE_FLASH.ld'
MAPFILE = 'ikv.map'

LDFLAGS = [
  '--static',
  '-nostartfiles',
  '-T'+LDFILE,
  '-Wl,-Map=ikv.map',
  '-Wl,--gc-sections',
  '-mthumb',
  '-mcpu=cortex-m3',
  '-msoft-float',
  '-mfix-cortex-m3-ldrd',
  '-Wl,--start-group',
  '-lc',
  '-lgcc',
  '-lnosys',
  '-Wl,--end-group',
  ]

CFILES = all_files('.')
STARTUPFILE = 'src/startup_stm32f2xx.s'
compile_options = {}
compile_options['CPPFLAGS'] = CFLAGS
compile_options['LINKFLAGS'] = LDFLAGS

obj = env.Object(source=CFILES, CPPPATH=INCLUDE_PATH, **compile_options)
startup = env.Object(source=STARTUPFILE, **compile_options)
prg = env.Program('ikv',obj+startup,**compile_options)
bin = env.Objcopy('ikv.bin',prg, TYPE = 'binary')
hex = env.Objcopy('ikv.hex',prg, TYPE = 'ihex')


#Object(CFILES, CCFLAGS = CFLAGS)
