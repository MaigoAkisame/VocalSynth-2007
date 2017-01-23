#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef char STRING[255];

// Directory specifications

#define WorkDir "../"
STRING DatabaseDir,SyllableDir,ScoreDir,AccomDir,SynthDir;
#define SyllableFileName "list_out.dat"

// Constants & simple functions

#define MaxSyllableLen 3
#define MaxSyllableCount 999
#define MaxBlockCount 999999
#define MaxUnitCount 999
#define FreqA4 440
#define LevelA4 57
short ToneLevel[7]={9,11,0,2,4,5,7};
char ToneChar[14]="1!2@34$5%6^70";
#define chr2int(x) (((x)<0)?((x)+256):(x))

// Wave file info

long FileSize;
char ChunkID[5];
long ChunkSize;
short FormatTag;
unsigned short ChannelCount;
unsigned long SamplesPerSec;
unsigned long BytesPerSec;
unsigned short BytesPerBlock;
unsigned short BitsPerSample;
long BlocksPerSec;
long BlockCount,BlockCount2,BlockCount3;
typedef short DATA[MaxBlockCount][2];
DATA data,data2;

// Syllables info

long SyllableCount;
typedef struct {
  char name[MaxSyllableLen+1];
  double duration;
  double consonant;
  double freq;
  double average[2];
  char file[32];
} SYLLABLE;
SYLLABLE Syllable[MaxSyllableCount];

// Score Parameters

long KeyLevel;
long BeatNumer,BeatDenom;
double Tempo;
long Quantize;
double AccomAmplify;
double VoiceAmplify;
long CurNote,CurSyllable=0,CurLen=0,Prolong;

// Other global variables

FILE *ScoreFile,*AccomFile,*SynthFile;

// File-operating functions

void ReadFile(STRING dir,STRING file,FILE **FilePtr) {
  STRING FileName;
  strcpy(FileName,dir);strcat(FileName,file);
  if ((*FilePtr=fopen(FileName,"rb"))==NULL) {
    printf("Error: Cannot open file %s!\n",FileName);
    exit(0);
  }
}

void WriteFile(STRING dir,STRING file,FILE **FilePtr) {
  STRING FileName;
  strcpy(FileName,dir);strcat(FileName,file);
  *FilePtr=fopen(FileName,"wb");
}

void Readln(FILE *file,char *s) {
  while ((*s=fgetc(file))>=' ') s++;
  if (*s!='\n') while (fgetc(file)!='\n');
  *s='\0';
}

// Syllable-related functions

void LoadSyllables() {
  FILE *SyllableFile;
  printf("Loading syllable list...\n");
  ReadFile(SyllableDir,SyllableFileName,&SyllableFile);
  fread(&SyllableCount,4,1,SyllableFile);
  fread(&Syllable[1],sizeof(SYLLABLE),SyllableCount,SyllableFile);
  fclose(SyllableFile);
}

long GetSyllableID(char *nameptr,long len) {
  char name[MaxSyllableLen+1];
  long i;
  for (i=0;i<len && nameptr[i]!=' ';i++) name[i]=nameptr[i];
  name[i]='\0';
  long l=1,r=SyllableCount,m,flag;
  while (l<=r && (flag=strcmp(Syllable[m=(l+r)/2].name,name))!=0)
    if (flag>0) r=m-1;else l=m+1;
  if (l>r) {printf("  Error: Unknown syllable \"%s\" encountered!\n",name);exit(0);}
  while (m>1 && strcmp(Syllable[m].name,Syllable[m-1].name)==0) m--;
  return m;
}

// Wave-loading functions

void ReadHeader(FILE *Wave) {
  fread(&ChunkID,4,1,Wave);
  if (strcmp(ChunkID,"RIFF")!=0) {fclose(Wave);printf("  Error: Missing \"RIFF\"!\n");exit(0);}
  fread(&FileSize,4,1,Wave);
  fread(&ChunkID,4,1,Wave);
  if (strcmp(ChunkID,"WAVE")!=0) {fclose(Wave);printf("  Error: Missing \"WAVE\"!\n");exit(0);}
}

void ReadFmtChunk(FILE *Wave) {
  long i;
  fread(&FormatTag,2,1,Wave);
  if (FormatTag!=1) {fclose(Wave);printf("  Error: Cannot process Non-Linear-PCM wave file.\n");exit(0);}
  fread(&ChannelCount,2,1,Wave);
  fread(&SamplesPerSec,4,1,Wave);
  fread(&BytesPerSec,4,1,Wave);
  fread(&BytesPerBlock,2,1,Wave);
  fread(&BitsPerSample,2,1,Wave);
  BlocksPerSec=BytesPerSec/BytesPerBlock;
  for (i=17;i<=ChunkSize;i++) fgetc(Wave);
}

void ReadFactChunk(FILE *Wave) {
  long i;
  for (i=1;i<=ChunkSize;i++) fgetc(Wave);
}

void ReadListChunk(FILE *Wave) {
  long i;
  for (i=1;i<=ChunkSize;i++) fgetc(Wave);
}

void ReadDataChunk(FILE *Wave) {
  long i;
  char tmp[2];
  BlockCount=ChunkSize/BytesPerBlock;
  switch (BitsPerSample) {
    case 8: switch (ChannelCount) {
              case 1:for (i=0;i<BlockCount;i++) {fread(tmp,1,1,Wave);data[i][0]=chr2int(tmp[0]);}break;
              case 2:for (i=0;i<BlockCount;i++) {fread(tmp,1,2,Wave);data[i][0]=chr2int(tmp[0]);data[i][1]=chr2int(tmp[1]);}
            }
            break;
    case 16:switch (ChannelCount) {
              case 1:for (i=0;i<BlockCount;i++) fread(data[i],2,1,Wave);break;
              case 2:fread(data,2,2*BlockCount,Wave);
            }
  }
}

void SkipChunk(FILE *Wave) {
  long i;
  for (i=1;i<=ChunkSize;i++) fgetc(Wave);
}

void LoadWave(STRING FileName) {
  FILE *Wave;
  ReadFile(SyllableDir,FileName,&Wave);
  ReadHeader(Wave);
  while (fread(&ChunkID,4,1,Wave)) {
    fread(&ChunkSize,4,1,Wave);
    if (strcmp(ChunkID,"fmt ")==0) ReadFmtChunk(Wave);
      else if (strcmp(ChunkID,"fact")==0) ReadFactChunk(Wave);
      else if (strcmp(ChunkID,"LIST")==0) ReadListChunk(Wave);
      else if (strcmp(ChunkID,"data")==0) ReadDataChunk(Wave);
      else SkipChunk(Wave);
  }
  fclose(Wave);
}

void LoadAccom() {
  ReadHeader(AccomFile);
  while (fread(&ChunkID,4,1,AccomFile)) {
    fread(&ChunkSize,4,1,AccomFile);
    if (strcmp(ChunkID,"fmt ")==0) ReadFmtChunk(AccomFile);
      else if (strcmp(ChunkID,"fact")==0) ReadFactChunk(AccomFile);
      else if (strcmp(ChunkID,"LIST")==0) ReadListChunk(AccomFile);
      else if (strcmp(ChunkID,"data")==0) break;
      else SkipChunk(AccomFile);
  }
  fwrite("RIFF",4,1,SynthFile);
  long t=36+ChunkSize;fwrite(&t,4,1,SynthFile);
  fwrite("WAVE",4,1,SynthFile);
  fwrite("fmt ",4,1,SynthFile);
  t=16;fwrite(&t,4,1,SynthFile);
  fwrite(&FormatTag,2,1,SynthFile);
  fwrite(&ChannelCount,2,1,SynthFile);
  fwrite(&SamplesPerSec,4,1,SynthFile);
  fwrite(&BytesPerSec,4,1,SynthFile);
  fwrite(&BytesPerBlock,2,1,SynthFile);
  fwrite(&BitsPerSample,2,1,SynthFile);
  fwrite("data",4,1,SynthFile);
  fwrite(&ChunkSize,4,1,SynthFile);
}

// Functions for synthesizing a single note

void Stretch(double OrigFreq,double StartTime,double EndTime,double Ratio) {
  long OrigPeriodID,AdaptedPeriodID;
  double BlocksPerPeriod=BlocksPerSec/OrigFreq;
  long IntBlocksPerPeriod=(long)BlocksPerPeriod+1,
       OrigStartPeriod=(long)(StartTime*OrigFreq),
       OrigEndPeriod=(long)(EndTime*OrigFreq);
  for (AdaptedPeriodID=0;(OrigPeriodID=(long)(OrigStartPeriod+AdaptedPeriodID/Ratio))<=OrigEndPeriod;AdaptedPeriodID++)
    memmove(data2[BlockCount2+(long)(BlocksPerPeriod*AdaptedPeriodID)],data[(long)(BlocksPerPeriod*OrigPeriodID)],4*IntBlocksPerPeriod);
  BlockCount2+=(long)(BlocksPerSec*(EndTime-StartTime)*Ratio);
}

void Resample(double Interval) {
  double SampleBlock=0,lambda;
  long j,IntSampleBlock;
  for (BlockCount3=0;SampleBlock<BlockCount2-1;BlockCount3++,SampleBlock+=Interval) {
    IntSampleBlock=(long)SampleBlock;lambda=SampleBlock-IntSampleBlock;
    for (j=0;j<ChannelCount;j++)
      data[BlockCount3][j]=(short)(data2[IntSampleBlock][j]+(data2[IntSampleBlock+1][j]-data2[IntSampleBlock][j])*lambda);
  }
}

void Pass(double Duration) {
  char tmp8;short tmp16;long tmp,i;
  switch (BitsPerSample) {
    case 8:for (i=1;i<=Duration*BlocksPerSec*ChannelCount;i++) {
             if (fread(&tmp8,1,1,AccomFile)<=0) break;
             tmp=(long)((chr2int(tmp8)-128)*AccomAmplify+128);
             if (tmp>255) tmp=255;else if (tmp<0) tmp=0;
             tmp8=(char)tmp;
             fwrite(&tmp8,1,1,SynthFile);
           }
           break;
    case 16:for (i=1;i<=Duration*BlocksPerSec*ChannelCount;i++) {
              if (fread(&tmp16,2,1,AccomFile)<=0) break;
              tmp=(long)(tmp16*AccomAmplify);
              if (tmp>32767) tmp=32767;else if (tmp<-32768) tmp=-32768;
              tmp16=(short)tmp;
              fwrite(&tmp16,2,1,SynthFile);
            }
  }
}

void SynthNote() {
  long i,j;
  double SynthDuration=(double)CurLen/Quantize*4/Tempo,DurationRatio;
  if (CurNote==0) {Pass(SynthDuration);return;}

  double SynthFreq=FreqA4*pow(2,(double)(CurNote-LevelA4)/12);
  while (CurSyllable<=SyllableCount && strcmp(Syllable[CurSyllable].name,Syllable[CurSyllable+1].name)==0 && Syllable[CurSyllable+1].freq<=SynthFreq) CurSyllable++;
  double FreqRatio=SynthFreq/Syllable[CurSyllable].freq;
  LoadWave(Syllable[CurSyllable].file);
  BlockCount2=0;
  if (Prolong) {
    DurationRatio=SynthDuration/(Syllable[CurSyllable].duration-Syllable[CurSyllable].consonant);
    Stretch(Syllable[CurSyllable].freq,Syllable[CurSyllable].consonant,Syllable[CurSyllable].duration,DurationRatio*FreqRatio);
  }
  else if (SynthDuration<Syllable[CurSyllable].duration) {
    DurationRatio=SynthDuration/Syllable[CurSyllable].duration;
    Stretch(Syllable[CurSyllable].freq,0,Syllable[CurSyllable].duration,DurationRatio*FreqRatio);
  }
  else {
    DurationRatio=(SynthDuration-Syllable[CurSyllable].consonant)/(Syllable[CurSyllable].duration-Syllable[CurSyllable].consonant),
    Stretch(Syllable[CurSyllable].freq,0,Syllable[CurSyllable].consonant,FreqRatio);
    Stretch(Syllable[CurSyllable].freq,Syllable[CurSyllable].consonant,Syllable[CurSyllable].duration,DurationRatio*FreqRatio);
  }
  Resample(FreqRatio);

  char tmp8;short tmp16;long tmp;
  for (i=0;i<BlockCount3;i++)
    for (j=0;j<ChannelCount;j++)
      switch (BitsPerSample) {
        case 8:fread(&tmp8,1,1,AccomFile);
               tmp=(long)((chr2int(tmp8)-128)*AccomAmplify+(data[i][j]-Syllable[CurSyllable].average[j])*VoiceAmplify+128);
               if (tmp>255) tmp=255;else if (tmp<0) tmp=0;
               tmp8=(char)tmp;
               fwrite(&tmp8,1,1,SynthFile);
               break;
        case 16:fread(&tmp16,2,1,AccomFile);
                tmp=(long)(tmp16*AccomAmplify+(data[i][j]-Syllable[CurSyllable].average[j])*VoiceAmplify);
                if (tmp>32767) tmp=32767;else if (tmp<-32768) tmp=-32768;
                tmp16=(short)tmp;
                fwrite(&tmp16,2,1,SynthFile);
      }
}

// Score-command functions

void SetSinger() {
  STRING Param;
  Readln(ScoreFile,Param);
  char *s=Param;while (*s==' ') s++;
  strcpy(SyllableDir,DatabaseDir);
  strcat(SyllableDir,s);
  strcat(SyllableDir,"/output/");
  LoadSyllables();
}

void SetKey() {
  STRING Param;
  Readln(ScoreFile,Param);
  KeyLevel=0;
  for (unsigned long i=0;i<strlen(Param);i++)
    if (Param[i]>='A' && Param[i]<='G') KeyLevel+=ToneLevel[Param[i]-'A'];
      else if (Param[i]=='#') KeyLevel++;
      else if (Param[i]=='b') KeyLevel--;
      else if (Param[i]>='0' && Param[i]<='9') KeyLevel+=12*(Param[i]-48);
}

void SetBeat() {
  fscanf(ScoreFile,"%ld/%ld",&BeatNumer,&BeatDenom);
  while (fgetc(ScoreFile)!='\n');
}

void SetTempo() {
  fscanf(ScoreFile,"%lf",&Tempo);Tempo/=60;
  while (fgetc(ScoreFile)!='\n');
}

void SetQuantize() {
  fscanf(ScoreFile,"%ld",&Quantize);
  while (fgetc(ScoreFile)!='\n');
}

void SetAccomAmplify() {
  fscanf(ScoreFile,"%lf",&AccomAmplify);
  while (fgetc(ScoreFile)!='\n');
}

void SetVoiceAmplify() {
  fscanf(ScoreFile,"%lf",&VoiceAmplify);
  while (fgetc(ScoreFile)!='\n');
}

void RestTime() {
  double time;
  fscanf(ScoreFile,"%lf",&time);while (fgetc(ScoreFile)!='\n');
  Pass(time);
}

void RestBars() {
  double bars;
  fscanf(ScoreFile,"%lf",&bars);while (fgetc(ScoreFile)!='\n');
  Pass(bars*BeatNumer/BeatDenom*4/Tempo);
}

void Melody() {
  STRING Hi,Mid,Low,Lyric;
  while (fgetc(ScoreFile)!='\n');
  for (;;) {
    Readln(ScoreFile,Hi);
    if (strcmp(Hi,"End Melody")==0) break;
    Readln(ScoreFile,Mid);
    Readln(ScoreFile,Low);
    Readln(ScoreFile,Lyric);
    long len=strlen(Hi),p;
    while (strlen(Lyric)<len) strcat(Lyric," ");
    printf("%s\n%s\n%s\n%s\n",Hi,Mid,Low,Lyric);
    for (p=0;p<len;p++) printf("=");printf("\r");
    p=0;
    while (p<len) {
      char *t=strchr(ToneChar,Mid[p]);
      if (t==NULL) CurLen++;else {
        if (CurLen>0) SynthNote();
        if (Mid[p]=='0') CurNote=0;else CurNote=KeyLevel+(long)(t-ToneChar);
        if (CurNote>0) {
          if (Hi[p]=='.') CurNote+=12;else if (Hi[p]==':') CurNote+=24;
            else if (Low[p]=='`') CurNote-=12;else if (Low[p]==':') CurNote-=24;
          if (Lyric[p]==' ')
            Prolong=1;
          else {
            CurSyllable=GetSyllableID(Lyric+p,(Mid[p+2]=='&')?3:2);
            Prolong=0;
          }
        }
        CurLen=1;
      }
      if (Mid[p+2]=='&') {printf(">>>");p+=3;} else {printf(">>");p+=2;}
    }
    printf("\n");
  }
  SynthNote();CurLen=0;
}

// Other functions

void ParseScore() {
  STRING Command;
  while (fscanf(ScoreFile,"%s",Command)>0) {
    if (strcmp(Command,"Singer")==0) SetSinger();
      else if (strcmp(Command,"Key")==0) SetKey();
      else if (strcmp(Command,"Beat")==0) SetBeat();
      else if (strcmp(Command,"Tempo")==0) SetTempo();
      else if (strcmp(Command,"Quantize")==0) SetQuantize();
      else if (strcmp(Command,"AccomAmplify")==0) SetAccomAmplify();
      else if (strcmp(Command,"VoiceAmplify")==0) SetVoiceAmplify();
      else if (strcmp(Command,"RestTime")==0) RestTime();
      else if (strcmp(Command,"RestBars")==0) RestBars();
      else if (strcmp(Command,"Melody")==0) Melody();
      else while(fgetc(ScoreFile)!='\n');
  }
  Pass(9999);
}

void Synthetize(STRING SongName) {
  STRING s;
  printf("Synthesizing song \"%s\"...\n",SongName);
  strcpy(s,SongName);strcat(s,".txt");ReadFile(ScoreDir,s,&ScoreFile);
  strcpy(s,SongName);strcat(s,".wav");ReadFile(AccomDir,s,&AccomFile);
  strcpy(s,SongName);strcat(s,"_synth.wav");WriteFile(SynthDir,s,&SynthFile);
  LoadAccom();
  ParseScore();
  fclose(ScoreFile);fclose(AccomFile);fclose(SynthFile);
  printf("Congratulations: Song \"%s\" synthetized successfully.\n\n",SongName);
}

void main(int argc,char *argv[]) {
  strcpy(DatabaseDir,WorkDir);strcat(DatabaseDir,"database/");
  strcpy(AccomDir,WorkDir);strcat(AccomDir,"music/accom/");
  strcpy(ScoreDir,WorkDir);strcat(ScoreDir,"music/score/");
  strcpy(SynthDir,WorkDir);strcat(SynthDir,"music/synth/");

  STRING SongName;
  if (argc>1)
    for (int i=1;i<argc;i++)
      Synthetize(argv[i]);
  else
    for (;;) {
      printf("Input name of song (blank line for finish): ");
      gets(SongName);
      if (strcmp(SongName,"")==0) break;
      Synthetize(SongName);
    }
}
