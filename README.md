# BoxScore
The latest development is focus on Objectization of the program.
The result is BoxScoreXY. 

The general idea are that
- All digitizer channel settings should be edited in the setting_*.txt
- All general digitizer settings should be edited in the generalSetting.txt (although obsolete now)
- Channel Mask, location, Canvas, Histogram, data processing should be done in the Plane class.
- Minimization for editing the BoxScoreXY.c
- Objectization makes the program more easy to maintain, debug, and develop.

## Class

- DigitizerClass.h
    - This class provides all the methods for handling digitizer, getting thr data, and event building. One thing need to fix is that the boardID will changed. That may cause mishandleing when multiple digitizers are being opened.
    - The generalSetting.txt is kind of obsolete. becasue the waveform is not read and the coincident Time window can be changed during the program.
    - The setting_X.txt is the place for channel setting.
- FileIO.h
    - This class handle root tree, histogram, and setting files saving.
- GenericPlane.h (Plane Class)
    - This class setup the basics need for Canvas and Histograms. It also stores the ChannelMask, database tag.
    - This class also handle how the data processing. The digitizer always output raw event based on channel. 
    - This class also handle how the histograms is being filled.
- HelioTarget.h (Plane Class)
    - This is an example for a derivative class for GenericPlane.

## BoxScoreXY
The BoxScoreXY is the meeting place for all classes.
1. The Plane class is first declare. 
2. The digitizer will be open, using the channel mask from the Plane class.
3. The Plane class will load some digitizer setting for histogram setting, such as the channel gain.
4. A keyboard detection loop will be started.

## Creating new Plane Class
There are few things to pay attension on creating a new Plane Class from GenericPlane.h
1. make sure you change the Plane class in BoxScoreXY.C
2. sometimes, you need to add some code for special histogram filling.
3. please add-back some now methods from the derived class in GenericPlane.h

## TODO list
- read waveform
- generic method to save all histograms
- read multiple digitizers ( require sycn )
- more generic method for read/write digitizer setting
