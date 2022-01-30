# "Spell or Speak" 

an alternative Firmware for the AE Modular GRAINS module by tangible waves

https://www.tangiblewaves.com/store/p86/GRAINS.html

The synthetic words, as used with a famous toy from the late 70th, can be triggered via Gate-impulses and selcted via CV or a pot on the GRAINS
  
To set up the environment needed to install this firmware, please refer to the AeManual for GRAINS on the AE Modular Wiki: http://wiki.aemodular.com/pmwiki.php/AeManual/GRAINS

* __Demotrack__ for Grains 'Spell or Speak' is available here: https://soundcloud.com/taitekatto/grains-spell-or-speak
  
This program relies on the Talkie Library by Peter Knight as modified by Jean-Luc Deladrière with "Talko Counter for Grains", which is also included in the folder of this firmware. The orinal "TALKO COUNTER" also including talkie.cpp and talkie.h of this version of the Talkie Library can be found here: https://www.ginkosynthese.com/grains-codes
The phonems used with this firmware as provided via "Vocab_US_TI99.c" and others can be found with the talkie-library for Arduino here: https://www.arduinolibraries.info/libraries/talkie 

This program was modified by the Reductionist Earth Catalog to be MIDI-controlled
  
## Note

If you want to adjust the way Pitch and Speed of the talking-voice can be modified please have a look at "talkie.cpp" in the current folder and look for the values mapped via "analogRead(0)" and "analogRead(1)".

# Usage of this Firmware with GRAINS:

__Inputs__

* IN1 / Pot1: Select word from available phonems (via CV or pot)
* IN2 / Pot2: Change the Pitch of the currently selected word (via CV or pot)
* Pot3:       Change the Speed of the currently selected word (via pot)

__Outputs__

* OUT:        Audio out of spoken words or letters
* D:          Quantized value (HIGH or LOW) of the input given via analog input, this may be used as GATE/trigger for other AE Modular modules

__Caution!__ Use at your own risk  (according to GNU General Public License v2 '12. No Warrenty')

# Tutorial Video
[![Tutorial for SpellOrSpeak](https://res.cloudinary.com/marcomontalbano/image/upload/v1586675057/video_to_markdown/images/youtube--a1aS6E0b0Fk-c05b58ac6eb4c4700831b2b3070cd403.jpg)](https://youtu.be/a1aS6E0b0Fk "Tutorial for SpellOrSpeak")
-------------------------------------------------------------  

This program in combination with the hardware it is applied to can produce harsh and loud frequencies that may be of harm to speakers or your ears! Permanent hearing loss may result from exposure to sound at high volumes. Use as low a volume as possible.

'Spell or Speak' an alternative Firmware for the AE Modular GRAINS module by tangible waves inspired by a famous toy from the 70th

Copyright (C) 2020  Mathias Brüssel

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

