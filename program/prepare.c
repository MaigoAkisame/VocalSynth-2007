#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef char STRING[255];

// Directory specifications

#define WorkDir "../database/"
#define InputListFileName "list_in.txt"
#define OutputListFileName "list_out.dat"
STRING InputDir,OutputDir;

// Constants & simple functions

#define MaxSyllableLen 3
#define MaxSyllableCount 999
#define MaxBlockCount 9999999
#define MaxUnitCount 999
#define MIN (short)0x8000
#define MAX (short)0x7FFF
#define chr2int(x) (((x)<0)?((x)+256):(x))

// Wave-segmenting parameters

#define MinUnitDuration 0.1
#define MinRestDuration 0.1
#define MinNoiseProportion 0
#define MaxNoiseProportion 0.2
#define StepNoiseProportion 0.01

// Wave-measuring parameters

double MinFreq,MaxFreq;
#define StepFreq 1
#define SamplesPerPeriod 50
#define MeasurePointCount 5
#define DurationCoefficient 0.99
  // In order to eliminate tail blank

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
long BlockCount;
short data[MaxBlockCount][2];
short MinOffset[2],MaxOffset[2];
double Average[2],Amplitude[2];

// Units info

long UnitStart[MaxUnitCount],UnitEnd[MaxUnitCount],UnitCount;

// Syllables info

long SyllableCount,TotalSyllableCount;
typedef struct {
  char name[MaxSyllableLen+1];
  double duration;
  double consonant;
  double freq;
  double average[2];
  char file[32];
} SYLLABLE;
SYLLABLE Syllable[MaxSyllableCount];

// File-opening functions

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

// Syllable-related functions

void LoadSyllables(STRING FileName) {
  FILE *Text;
  ReadFile(InputDir,FileName,&Text);
  SyllableCount=0;
  while (fscanf(Text,"%s",Syllable[TotalSyllableCount+(++SyllableCount)].name)>0);SyllableCount--;
  printf("  Found %ld syllables.\n",SyllableCount);
  TotalSyllableCount+=SyllableCount;
  fclose(Text);
}

long GetSyllableID(char *name) {
  long l=1,r=TotalSyllableCount,m,flag;
  while (l<=r && (flag=strcmp(Syllable[m=(l+r)/2].name,name))!=0)
    if (flag>0) r=m-1;else l=m+1;
  while (m>1 && strcmp(Syllable[m].name,Syllable[m-1].name)==0) m--;
  return m;
}

void SortSyllables(long s,long t) {
  long p,i,j,cmp;
  SYLLABLE tmp;
  if (s>=t) return;
  p=s+rand()%(t-s+1);
  tmp=Syllable[p];Syllable[p]=Syllable[s];
  i=s;j=t;
  while (i<j) {
    while (i<j && ((cmp=strcmp(Syllable[j].name,tmp.name))>0 || cmp==0 && Syllable[j].freq>tmp.freq)) j--;
    if (i==j) break;Syllable[i]=Syllable[j];i++;
    while (i<j && ((cmp=strcmp(Syllable[i].name,tmp.name))<0 || cmp==0 && Syllable[i].freq<tmp.freq)) i++;
    if (i==j) break;Syllable[j]=Syllable[i];j--;
  }
  Syllable[i]=tmp;
  SortSyllables(s,i-1);
  SortSyllables(i+1,t);
}

void SaveSyllables() {
  FILE *OutputList;
  printf("Saving Syllables...\n");
  WriteFile(OutputDir,OutputListFileName,&OutputList);
  fwrite(&TotalSyllableCount,4,1,OutputList);
  fwrite(&Syllable[1],sizeof(SYLLABLE),TotalSyllableCount,OutputList);
  fclose(OutputList);
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
  printf("Reading Format Chunk...\n");
  fread(&FormatTag,2,1,Wave);
  if (FormatTag!=1) {fclose(Wave);printf("  Error: Cannot process Non-Linear-PCM wave file.\n");exit(0);}
  fread(&ChannelCount,2,1,Wave);
  fread(&SamplesPerSec,4,1,Wave);
  fread(&BytesPerSec,4,1,Wave);
  fread(&BytesPerBlock,2,1,Wave);
  fread(&BitsPerSample,2,1,Wave);
  BlocksPerSec=BytesPerSec/BytesPerBlock;
  for (i=17;i<=ChunkSize;i++) fgetc(Wave);
  printf("  Channels:             %s\n",(ChannelCount==1)?"1 (Mono)":"2 (Stereo)");
  printf("  Samples per second:   %ld\n",SamplesPerSec);
  printf("  Bytes per second:     %ld\n",BytesPerSec);
  printf("  Block size:           %d\n",BytesPerBlock);
  printf("  Bits per sample:      %d\n",BitsPerSample);
  printf("  Blocks per second:    %.0f\n",BlocksPerSec);
}

void ReadFactChunk(FILE *Wave) {
  long i;
  printf("Skipping Fact Chunk...\n");
  for (i=1;i<=ChunkSize;i++) fgetc(Wave);
}

void ReadListChunk(FILE *Wave) {
  long i;
  printf("Skipping List Chunk...\n");
  for (i=1;i<=ChunkSize;i++) fgetc(Wave);
}

void ReadDataChunk(FILE *Wave) {
  long i;
  char tmp[2];
  printf("Reading Data Chunk...\n");
  BlockCount=ChunkSize/BytesPerBlock;
  printf("  Total blocks:         %d\n",BlockCount);
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
  printf("  Warning: Unknown chunk encountered and ignored.\n");
  for (i=1;i<=ChunkSize;i++) fgetc(Wave);
}

void LoadWave(STRING FileName) {
  FILE *Wave;
  ReadFile(InputDir,FileName,&Wave);
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

// Wave-saving functions

void SaveWave(long StartBlock,long EndBlock,STRING FileName) {
  FILE *Wave;
  long j,t;
  char tmp[2];

  WriteFile(OutputDir,FileName,&Wave);
  fwrite("RIFF",4,1,Wave);
  t=36+BytesPerBlock*(EndBlock-StartBlock+1);fwrite(&t,4,1,Wave);
  fwrite("WAVE",4,1,Wave);
  fwrite("fmt ",4,1,Wave);
  t=16;fwrite(&t,4,1,Wave);
  fwrite(&FormatTag,2,1,Wave);
  fwrite(&ChannelCount,2,1,Wave);
  fwrite(&SamplesPerSec,4,1,Wave);
  fwrite(&BytesPerSec,4,1,Wave);
  fwrite(&BytesPerBlock,2,1,Wave);
  fwrite(&BitsPerSample,2,1,Wave);
  fwrite("data",4,1,Wave);
  t=BytesPerBlock*(EndBlock-StartBlock+1);fwrite(&t,4,1,Wave);
  switch (BitsPerSample) {
    case 8: switch (ChannelCount) {
              case 1:for (j=StartBlock;j<=EndBlock;j++) {tmp[0]=(char)data[j][0];fwrite(tmp,1,1,Wave);}break;
              case 2:for (j=StartBlock;j<=EndBlock;j++) {tmp[0]=(char)data[j][0];tmp[1]=(char)data[j][1];fwrite(tmp,1,2,Wave);}
            }
            break;
    case 16:switch (ChannelCount) {
              case 1:for (j=StartBlock;j<=EndBlock;j++) fwrite(data[j],2,1,Wave);break;
              case 2:fwrite(data[StartBlock],2,2*(EndBlock-StartBlock+1),Wave);
            }
  }
  fclose(Wave);
}

// Wave-segmenting functions

void Segment(double NoiseProportion) {
  enum {REST,UNIT} status=REST;
  double flag;
  long count;
  long MinUnitBlocks=(long)(MinUnitDuration*BlocksPerSec),
       MinRestBlocks=(long)(MinRestDuration*BlocksPerSec);
  double MaxRestOffset[2],MinRestOffset[2];
  for (long j=0;j<ChannelCount;j++) {
    MaxRestOffset[j]=Average[j]+Amplitude[j]*NoiseProportion;
    MinRestOffset[j]=Average[j]-Amplitude[j]*NoiseProportion;
  }
  UnitCount=0;
  for (long i=0;i<BlockCount;i++) {
    switch (ChannelCount) {
      case 1:flag=(data[i][0]>MaxRestOffset[0] || data[i][0]<MinRestOffset[0]);break;
      case 2:flag=(data[i][0]>MaxRestOffset[0] || data[i][0]<MinRestOffset[0] ||
                   data[i][1]>MaxRestOffset[1] || data[i][1]<MinRestOffset[1]);
    }
    switch (status) {
      case REST:if (flag) {
                  UnitStart[++UnitCount]=i;status=UNIT;count=1;
                }
                break;
      case UNIT:if (flag) count=0;else if (++count==MinRestBlocks) {
                  UnitEnd[UnitCount]=i-MinRestBlocks;
                  if (UnitEnd[UnitCount]-UnitStart[UnitCount]+1<MinUnitBlocks) UnitCount--;
                  status=REST;
                }
    }
  }
  if (status==UNIT) {
    UnitEnd[UnitCount]=BlockCount-1-count;
    if (UnitEnd[UnitCount]-UnitStart[UnitCount]+1<MinUnitBlocks) UnitCount--;
  }
}

// Wave-measuring functions

double MeasureDuration(long StartBlock,long EndBlock) {
  return (double)(EndBlock-StartBlock)/BlocksPerSec;
}

double Offset(double time,long channel) {
  double lambda=time*BlocksPerSec;
  long BlockID=(long)lambda;lambda-=BlockID;
  return data[BlockID][channel]+(data[BlockID+1][channel]-data[BlockID][channel])*lambda;
}

double MeasureFreq(long StartBlock,long EndBlock) {
  double MeasurePoint,Freq,Period,Time,Diff,MinDiff,Offset1,Offset2,Offset3,t;
  double ResFreq[MeasurePointCount+1];
  long h,i,j,p;
  for (h=1;h<=MeasurePointCount;h++) {
    MeasurePoint=(StartBlock+(EndBlock-StartBlock)*(double)h/(MeasurePointCount+1))/BlocksPerSec;
    MinDiff=-1;
    for (Freq=MinFreq;Freq<=MaxFreq;Freq+=StepFreq) {
      Period=1/Freq;
      Diff=0;
      for (j=0;j<ChannelCount;j++)
        for (i=0;i<SamplesPerPeriod;i++) {
          Time=MeasurePoint+(-0.5+(i+0.5)/SamplesPerPeriod)*Period;
          Offset1=Offset(Time-Period,j);
          Offset2=Offset(Time,j);
          Offset3=Offset(Time+Period,j);
          Diff+=fabs(Offset1-Offset2)+fabs(Offset2-Offset3);
        }
      if (MinDiff<0 || Diff<MinDiff) {MinDiff=Diff;ResFreq[h]=Freq;}
    }
  }
  for (i=1;i<MeasurePointCount;i++) {
    p=i;
    for (j=i+1;j<=MeasurePointCount;j++)
      if (ResFreq[j]<ResFreq[p]) p=j;
    if (p!=i) {t=ResFreq[i];ResFreq[i]=ResFreq[p];ResFreq[p]=t;}
  }
  if (MeasurePointCount%2) return ResFreq[MeasurePointCount/2+1];
    else return (ResFreq[MeasurePointCount/2]+ResFreq[MeasurePointCount/2+1])/2;
}

short MeasureMin(long Channel,long StartBlock,long EndBlock) {
  short min=MAX;
  for (long i=StartBlock;i<=EndBlock;i++)
    if (data[i][Channel]<min) min=data[i][Channel];
  return min;
}

short MeasureMax(long Channel,long StartBlock,long EndBlock) {
  short max=MIN;
  for (long i=StartBlock;i<=EndBlock;i++)
    if (data[i][Channel]<max) max=data[i][Channel];
  return max;
}

double MeasureAverage(long Channel,long StartBlock,long EndBlock) {
  double sum=0;
  for (long i=StartBlock;i<=EndBlock;i++)
    sum+=data[i][Channel];
  return sum/(EndBlock-StartBlock+1);
}

// Other functions

void Process(STRING InputTextFileName,STRING InputWaveFileName) {
  long i,j;
  printf("Processing text file %s & wave file %s...\n",InputTextFileName,InputWaveFileName);

  // Load
  LoadWave(InputWaveFileName);
  LoadSyllables(InputTextFileName);

  // Segment
  for (j=0;j<ChannelCount;j++) {
    MinOffset[j]=MeasureMin(j,0,BlockCount-1);
    MaxOffset[j]=MeasureMax(j,0,BlockCount-1);
    Average[j]=MeasureAverage(j,0,BlockCount-1);
    Amplitude[j]=(MaxOffset[j]-Average[j]>Average[j]-MinOffset[j])?(MaxOffset[j]-Average[j]):(Average[j]-MinOffset[j]);
  }
  double NoiseProportion;
  for (NoiseProportion=MaxNoiseProportion;NoiseProportion>=MinNoiseProportion;NoiseProportion-=StepNoiseProportion) {
    Segment(NoiseProportion);
    if (UnitCount==SyllableCount) break;
  }
  if (UnitCount!=SyllableCount) {printf("Error: Unable to segment %s.\n",InputWaveFileName);exit(0);}
  printf("  Found %ld unit(s) using tolerable noise proportion %ld%%.\n",UnitCount,(long)(NoiseProportion*100));

  // Measure & Save
  for (i=1;i<=UnitCount;i++) {
    long SyllableIndex=TotalSyllableCount-SyllableCount+i;
    Syllable[SyllableIndex].duration=MeasureDuration(UnitStart[i],UnitEnd[i])*DurationCoefficient;
    // Determine the consonant duration of the syllable. Not a good algo.
    switch (Syllable[SyllableIndex].name[0]) {
      case 's':case 'z':case 'j':case 'c':Syllable[SyllableIndex].consonant=0.2;break;
      default:Syllable[SyllableIndex].consonant=0.1;
    }
    if (Syllable[SyllableIndex].consonant>Syllable[SyllableIndex].duration/2)
      Syllable[SyllableIndex].consonant=Syllable[SyllableIndex].duration/2;
    // Consonant duration determination finished
    Syllable[SyllableIndex].freq=MeasureFreq(UnitStart[i],UnitEnd[i]);
    for (j=0;j<ChannelCount;j++)
      Syllable[SyllableIndex].average[j]=MeasureAverage(j,UnitStart[i],UnitEnd[i]);
    strcpy(Syllable[SyllableIndex].file,InputWaveFileName);
    strcpy(Syllable[SyllableIndex].file+strlen(Syllable[SyllableIndex].file)-4,"_");
    strcpy(Syllable[SyllableIndex].file+strlen(Syllable[SyllableIndex].file),Syllable[SyllableIndex].name);
    strcpy(Syllable[SyllableIndex].file+strlen(Syllable[SyllableIndex].file),".wav");
    printf("  Syllable %ld\n",i);
    printf("    Name: %-3s    Filename:%s\n",Syllable[SyllableIndex].name,Syllable[SyllableIndex].file);
    printf("    Duration: %5.3fs    Frequency: %4.0fHz\n",
           Syllable[SyllableIndex].duration,Syllable[SyllableIndex].freq);
    SaveWave(UnitStart[i],UnitEnd[i],Syllable[SyllableIndex].file);
  }
}

void Prepare(STRING Singer) {
  FILE *InputList;
  STRING InputTextFileName,InputWaveFileName;
  strcpy(InputDir,WorkDir);strcat(InputDir,Singer);strcat(InputDir,"/input/");
  strcpy(OutputDir,WorkDir);strcat(OutputDir,Singer);strcat(OutputDir,"/output/");

  printf("Building database of singer \"%s\"...\n",Singer);
  ReadFile(InputDir,InputListFileName,&InputList);
  TotalSyllableCount=0;
  while (fscanf(InputList,"%s%s%lf%lf",InputTextFileName,InputWaveFileName,&MinFreq,&MaxFreq)>0)
    Process(InputTextFileName,InputWaveFileName);
  fclose(InputList);
  printf("Sorting Syllables (Total %ld)...\n",TotalSyllableCount);
  SortSyllables(1,TotalSyllableCount);
  SaveSyllables();
  printf("Congratulations: Database of singer \"%s\" built successfully.\n\n",Singer);
}

void main(int argc,char *argv[]) {
  STRING Singer;
  if (argc>1)
    for (int i=1;i<argc;i++)
      Prepare(argv[i]);
  else
    for (;;) {
      printf("Input name of singer (blank line for finish): ");
      gets(Singer);
      if (strcmp(Singer,"")==0) break;
      Prepare(Singer);
    }
}
