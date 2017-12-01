#!/usr/bin/env nodejs

var fs = require('fs');
var request = require('request');
var progress = require('request-progress');

var spec={
	processors:[
		{
			name:'rbdownload.download',
			exe_command:__filename+' download $(arguments)',
			version:0.1,
			inputs:[],
			outputs:[{name:'output',optional:false}],
			parameters:[{name:'sha1',optional:false}],
			opts:{
				cache_output:true
			}
		}
	]
};

function print_spec() {	
	console.log (JSON.stringify(spec,null,4));
	return;
}

var CLP=new CLParams(process.argv);
var arg1=CLP.unnamedParameters[0]||'';
if ((!arg1)||(arg1=='spec')) {
	print_spec();
}
else if (arg1=='download') {
	handle_download(CLP.namedParameters);
}
else {
	console.error('Unrecognized processor: '+arg1);
	process.exit(-1);
}

function handle_download(params) {
	var sha1=params.sha1;
	if (!sha1) {
		console.error('Missing parameter: sha1');
		return false;
	}
	var url='https://river.simonsfoundation.org/download/'+sha1;
	if (require('fs').existsSync(params.output))
		require('fs').unlinkSync(params.output);
	download_url(url,params.output,function(err) {
		if (err) {
			console.error(err);
			process.exit(-1);
		}
		process.exit(0);
	});
}

function download_url(url,path,callback) { 
	progress(request(url), {
	    throttle: 5000                    // Throttle the progress event (ms), defaults to 1000ms 
	    // delay: 1000,                       // Only start to emit after 1000ms delay, defaults to 0ms 
	    // lengthHeader: 'x-transfer-length'  // Length header to use, defaults to content-length 
	})
	.on('progress', function (state) {
	    // The state is an object that looks like this: 
	    // { 
	    //     percent: 0.5,               // Overall percent (between 0 to 1) 
	    //     speed: 554732,              // The download speed in bytes/sec 
	    //     size: { 
	    //         total: 90044871,        // The total payload size in bytes 
	    //         transferred: 27610959   // The transferred payload size in bytes 
	    //     }, 
	    //     time: { 
	    //         elapsed: 36.235,        // The total elapsed seconds since the start (3 decimals) 
	    //         remaining: 81.403       // The remaining seconds to finish (3 decimals) 
	    //     } 
	    // } 
	    console.log ('Downloaded '+(state.size.transferred/(1024*1024))+' of '+(state.size.total/(1024*1024))+' MB ('+Math.floor(state.percent*100)+'%)');
	    //console.log('progress', state);
	})
	.on('error', function (err) {
	    callback(err.message);
	    return;
	})
	.on('end', function () {
	    callback();
	})
	.pipe(fs.createWriteStream(path));
}

function CLParams(argv) {
	this.unnamedParameters=[];
	this.namedParameters={};

	var args=argv.slice(2);
	for (var i=0; i<args.length; i++) {
		var arg0=args[i];
		if (arg0.indexOf('--')===0) {
			arg0=arg0.slice(2);
			var ind=arg0.indexOf('=');
			if (ind>=0) {
				this.namedParameters[arg0.slice(0,ind)]=arg0.slice(ind+1);
			}
			else {
				//this.namedParameters[arg0]=args[i+1]||'';
				//i++;
				this.namedParameters[arg0]='';
			}
		}
		else if (arg0.indexOf('-')===0) {
			arg0=arg0.slice(1);
			this.namedParameters[arg0]='';
		}
		else {
			this.unnamedParameters.push(arg0);
		}
	}
};