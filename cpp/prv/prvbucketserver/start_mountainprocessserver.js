#!/usr/bin/env nodejs

var mountainprocessserver=require(__dirname+'/mountainprocessserver.js');
var http=require('http');

function print_usage() {
	console.log ('Usage: start_mountainprocessserver [data_directory] --listen_port=[port] [--listen_hostname=[host]]');
}

CLP=new CLParams(process.argv);
// The listen port comes from the command-line option
var listen_port=CLP.namedParameters['listen_port']||0;
var listen_hostname=CLP.namedParameters['listen_hostname']||'';
var mp_exe=__dirname+'/../../../bin/mountainprocess';
var mountainprocess_url_path='/mountainprocess';

// The first argument is the data directory -- the base path from which files will be served
var data_directory=CLP.unnamedParameters[0]||'';
if ((!data_directory)||(!listen_port)) {
	print_usage();
	process.exit(-1);
}

console.log ('Running mountainprocessserver...');

// Exit this program when the user presses ctrl+C
process.on('SIGINT', function() {
    process.exit();
});

// Create the web server!
var handler_opts={};
handler_opts.mp_exe=mp_exe;
handler_opts.mountainprocess_url_path=mountainprocess_url_path;
handler_opts.data_directory=data_directory;
var Handler=new mountainprocessserver.RequestHandler(handler_opts);
var SERVER=http.createServer(Handler.handle_request);
if (listen_hostname)
	SERVER.listen(listen_port,listen_hostname);
else
	SERVER.listen(listen_port);
SERVER.timeout=1000*60*60*24; //give it 24 hours!
console.log ('Listening on port: '+listen_port+', host: '+listen_hostname);

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
				this.namedParameters[arg0]=args[i+1]||'';
				i++;
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
}
