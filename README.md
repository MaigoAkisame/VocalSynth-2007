# VocalSynth-2007
This is a C program to synthesize Japanese vocal music that I wrote in 2007, even before I learnt the Fourier transform. Basically, the program takes a database of pre-recorded syllables, stretches each of them to match the pitch specified by the music score, and repeats or deletes some cycles to get the desired duration. The stretching is performed uniformly, which affects the formants. It was only later that I learnt of the TD-PSOLA (time domain pitch-synchronous overlap and add) algorithm which can preserve the formants.

The ```program``` folder contains two C programs: ```prepare.c``` and ```synth.c```:
* ```prepare.c``` prepares a database by segmenting a long recording into individual syllables.
* ```synth.c``` synthesizes songs from accompaniment files and score files.

You may compile these programs with your favorite compiler.

The ```database``` folder contains databases for different singers. Currently there are three singers:
* ```Tachiki```: A female voice produced by a synthesizer. Originally generated at 11025 Hz and 8 bits, then upsampled to 44100 Hz and 16 bits. Sounds very muffled; not recommended.
* ```Maigo_old```: My own voice reading all Japanese syllables at one pitch. Originally recorded at 11025 Hz and 8 bits, then upsampled to 44100 Hz and 16 bits. Because of the formant change that happens with the stretching, high-pitch notes synthesized with this database sound like a chipmunk.
* ```Maigo_new```: My own voice reading all Japanese syllables at four pitches. Directly recorded at 44100 Hz and 16 bits. Recommended.

You may also build databases with your own voice. Create your own singer folder, as well as two sub-folders ```input``` and ```output```. Record yourself reading syllables in one or more wav files, transcribe them in text files, and put both in the ```input``` folder. Also add a ```list_in.txt``` file associating each wav file with its transcription, as well as a range for the fundamental frequency. The you can run ```prepare.c``` to segment the recordings, the result of which will be written to the ```output``` folder.

The ```music``` folder contains accompaniment files (```accom```), music score files (```score```), and synthesized songs (```synth```). Three songs are provided as examples:
* 桜色の季節 (*The Cherry-Colored Season*), an original song by me;
* 春の呼び声 (*The Call of Spring*), an original song by me;
* 胸がドキドキ (*A Pounding Heart*), the first opening song of Detective Conan.

The accompaniment files must be in the same wav format as the database files. The music score files are plain-text files, and the format should be pretty intuitive if you are familiar with the *jianpu* (简谱) notation used in Chinese primary schools. A more detailed description of the format can be found in the report below.

The ```thesis``` folder contains a report of this project (which was originally a course project). Unfortunately it's in Chinese only. Chapter 4 is most relevant for understanding the code. (Forgive me for calling it a "thesis" -- that was my English in 2007)
