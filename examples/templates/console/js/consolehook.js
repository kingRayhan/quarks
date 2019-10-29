//consolehook.js
var drawFunc = null;
var printCommands = null;
var defaultPrompt = " ";

function format(out){
    return " " + out + " ";
}

var print = function(text, prompt){
    if (typeof prompt === 'undefined'){
        prompt = defaultPrompt;
    }
    if(printCommands){
        printCommands(format(text), prompt);
    }
    
    setTimeout(function(){
        if(drawFunc){
               drawFunc();
        }
    }, 10);

};

function scanResponse(text, pushCommand, drawer){
    pushCommand(format(text), defaultPrompt);
    
    setTimeout(function(){
               drawer();
    }, 300);
}

var onCRLF = function(commandsPusher, drawer){
    drawFunc = drawer;
    printCommands = commandsPusher.pushCommand;
    defaultPrompt = commandsPusher.defaultPrompt;
    
    scan(commandsPusher.lastCommand, function(responseText){
         if(responseText){
            scanResponse(responseText, commandsPusher.pushCommand, drawer);
         }
    });
    
   
};
