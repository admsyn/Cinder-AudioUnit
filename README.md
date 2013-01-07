Cinder-AudioUnit
============

Cinder block which simplifies hosting of [Audio Units](http://en.wikipedia.org/wiki/Audio_Units) in Cinder apps.

####Adding Cinder-AudioUnit to your App
This block makes use of the [recent app::rewrite branch of Cinder & TinderBox](https://github.com/cinder/Cinder/tree/appRewrite) to ease the creation of new Cinder apps. First, add this repository to your cinder/blocks folder:

    cd path/to/cinder/
    git clone git@github.com:admsyn/Cinder-AudioUnit.git blocks/AudioUnit

Then, you should see the block appear when you run TinderBox. If you need to add this block manually, just add this blocks source files and the following frameworks to your project:

1. CoreAudioKit.framework (for Audio Unit GUI support)
2. Carbon.framework (for Carbon-based GUIs)
3. CoreMidi.frameworks (Midi support)

####How to use this block
See the sample projects in the samples/ folder for examples. The **auBasic** sample runs though the Basics of Audio Unit hosting, and the **auComplexRouting** sample delves into more of the features.

####Copyright Notice & License
Copyright (c) 2013, Adam Carlucci - All rights reserved.
This code is intended for use with the Cinder C++ library: http://libcinder.org

Redistribution and use in source and binary forms, with or without modification, are permitted provided that
the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and  the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
