var fs = require("fs");
var parseMidi = require("midi-file").parseMidi;

var filename = process.argv[2];
var outputFileBaseName = filename.split(".").slice(0,-1).join(".");

var inputFile = fs.readFileSync(filename);
var parsedInput = parseMidi(inputFile);

var doPrints = false;

class GTNote {
    constructor(frames, noteNumber) {
        this.frames = Math.floor(frames) % 256;
        this.noteNumber = noteNumber;
        if(noteNumber == 0) {
            this.noteCode = 0;
        } else {
            var octaveNum = Math.floor(noteNumber/12)-1;
            var noteInOctave = noteNumber % 12;
            this.noteCode = (octaveNum * 16) + noteInOctave;
        }
    }
}

console.log(parsedInput.header);
var ticksPerBeat = parsedInput.header.ticksPerBeat;
var framesPerBeat = 32;
var microsecondsPerFrame = 16666.666667;

var ch0 = parsedInput.tracks[0];
var ch1 = parsedInput.tracks[1];
var ch2 = parsedInput.tracks[2];

var lastTrackLength = 0;

function processChannel(ch, outname) {
    var maxDelta = 0;
    var eventTypes = {};
    var noteTracker = {};
    var totalTicks = 0;
    var totalFrames = 0;
    var chNotes = [];
    var chLength = 0;
    var lastOff = 0;
    var biggestPause = 0;
    var notesOn = 0;
    var lastNoteOn = 0;
    var timeError = 0;
    var firstOn = -1;
    var noteCnt = 0;
    var restCnt = 0;
    var allError = 0;

    function doNoteOff(noteNumber) {
        if(noteTracker[noteNumber] >= 0) {
            notesOn--;
            lastOff = totalTicks;
            var noteTicks = totalTicks - noteTracker[noteNumber];
            var noteFrames = framesPerBeat * noteTicks / ticksPerBeat;
            if(noteFrames > 256) {
                console.log("TODO: add rest after unsupportedly long notes");
            }

            timeError += noteFrames - Math.floor(noteFrames);
            allError += noteFrames - Math.floor(noteFrames);
            noteFrames = Math.floor(noteFrames);
            //console.log("time error: " + timeError);
            noteFrames += Math.floor(timeError);
            timeError -= Math.floor(timeError);
            if(noteFrames >= 1) {
                chNotes.push(new GTNote(noteFrames, noteNumber));
                totalFrames += noteFrames;
                noteCnt++;
            }
            delete noteTracker[noteNumber];
        }
    }

    ch.forEach(function(evt, ind) {
        totalTicks += evt.deltaTime;
        if(evt.type == "noteOn") {
            if(firstOn == -1) {
                firstOn = totalTicks;
                console.log("first note at " + firstOn);
            }
            if(totalTicks == 6144) {
                console.log( totalTicks - lastOff - 1);
            }
            if(notesOn > 0) {
                doNoteOff(lastNoteOn);
            }
            notesOn++;
            var sinceLastOff = totalTicks - lastOff;
            if(sinceLastOff >= 1) {
                var restFrames = framesPerBeat * sinceLastOff / ticksPerBeat;
                if(totalTicks == 6144) {
                    console.log(framesPerBeat);
                    console.log(ticksPerBeat);
                    console.log(restFrames);
                }
                timeError += restFrames - Math.floor(restFrames);
                allError += restFrames - Math.floor(restFrames);
                restFrames = Math.floor(restFrames);
                //console.log("time error: " + timeError);
                restFrames += Math.floor(timeError);
                timeError -= Math.floor(timeError);
                while(restFrames >= 256) {
                    chNotes.push(new GTNote(256, 0)); //insert a max length rest
                    restFrames -= 256;
                    totalFrames += 256;
                    restCnt++;
                }
                if(restFrames >= 1) {
                    chNotes.push(new GTNote(restFrames, 0)); //insert a rest
                    totalFrames += restFrames;
                    restCnt++;
                }
            }
            if(totalTicks == 6144) {
                console.log("note at 6144");
                console.log("frames is " + totalFrames);
                console.log("error is " + allError);
                console.log(noteCnt + " notes, " + restCnt + " rests")
            }
            noteTracker[evt.noteNumber] = totalTicks;
            lastNoteOn = evt.noteNumber;
        } else if(evt.type == "noteOff") {
            doNoteOff(evt.noteNumber);
        } else if(evt.type == "endOfTrack") {
            chLength = Math.round(framesPerBeat * totalTicks / ticksPerBeat);
        } else if (evt.type == "setTempo") {
            framesPerBeat = Math.round(evt.microsecondsPerBeat / microsecondsPerFrame);
        }
        if(evt.deltaTime > maxDelta) {
            maxDelta = evt.deltaTime;
        }
        eventTypes[evt.type] = (eventTypes[evt.type] | 0) + 1;
        if(notesOn > 1) {
            console.log("note collision! " + JSON.stringify({
                eventNum : ind,
                time : totalTicks
            }));
        }
    });

    console.log(chNotes.length)
    console.log("0x" + chLength.toString(16));

    if(chLength > lastTrackLength) {
        lastTrackLength = chLength;
    }

    //var b = new Buffer(chNotes.length * 2);
    var b = Buffer.alloc(chNotes.length * 2);
    chNotes.forEach((item, ind) => {
        b[ind*2] = item.frames % 256;
        b[ind*2 + 1] = item.noteCode;
    });
    //fs.writeFileSync(outputFileBaseName + "_" + outname + ".gtm", b);
    //console.log("saved " + outputFileBaseName + "_" + outname + ".gtm");
    return b;
}

processChannel(ch0, "tempo");
var ch1b = processChannel(ch1, "ch1");
var ch2b = processChannel(ch2, "ch2");
var ch3b = processChannel(parsedInput.tracks[3]);
var ch4b = processChannel(parsedInput.tracks[4]);

var headerLen = 8; //header is two bytes for total song length in frames, then two bytes for offset of channel 2
var ch1Offset = headerLen; //redundant to include this, its immediately after the header of known size
var ch2Offset = 1 + ch1Offset + ch1b.length;
var ch3Offset = 1 + ch2Offset + ch2b.length;
var ch4Offset = 1 + ch3Offset + ch3b.length;

function highByte(num) {
    return (num & 0xFF00) >> 8;
}

function lowByte(num) {
    return num & 0xFF;
}

function bwrap(num) {
    var wrapperbuf = new Uint8Array(1);
    wrapperbuf[0] = num;
    return wrapperbuf;
}

var stream = fs.createWriteStream(outputFileBaseName + "_alltracks.gtm");

stream.on("error", console.error);
stream.write(bwrap(lowByte(lastTrackLength)));
stream.write(bwrap(highByte(lastTrackLength)));
stream.write(bwrap(lowByte(ch2Offset)));
stream.write(bwrap(highByte(ch2Offset)));
stream.write(bwrap(lowByte(ch3Offset)));
stream.write(bwrap(highByte(ch3Offset)));
stream.write(bwrap(lowByte(ch4Offset)));
stream.write(bwrap(highByte(ch4Offset)));
stream.write(bwrap(0));
stream.write(ch1b);
stream.write(bwrap(0));
stream.write(ch2b);
stream.write(bwrap(0));
stream.write(ch3b);
stream.write(bwrap(0));
stream.write(ch4b);
stream.end();
