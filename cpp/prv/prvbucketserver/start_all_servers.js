#!/usr/bin/env nodejs

var prvbucketserver=require(__dirname+'/prvbucketserver.js');
var mountainprocessserver=require(__dirname+'/mountainprocessserver.js');
var http=require('http');
var	url=require('url');

function print_usage() {
	console.log ('Usage: start_all_servers [data_directory] --listen_port=[port] --server_url=[http://localhost:8000] [--listen_hostname=[host]]');
}

CLP=new CLParams(process.argv);
// The listen port comes from the command-line option
var listen_port=CLP.namedParameters['listen_port']||0;
var listen_hostname=CLP.namedParameters['listen_hostname']||'';
var server_url=CLP.namedParameters['server_url']||'';
var prv_exe=__dirname+'/../../../bin/prv';
var mp_exe=__dirname+'/../../../bin/mountainprocess';
var prvbucket_url_path='/prvbucket';
var mountainprocess_url_path='/mountainprocess';

// The first argument is the data directory -- the base path from which files will be served
var data_directory=CLP.unnamedParameters[0]||'';
if ((!data_directory)||(!listen_port)||(!('server_url' in CLP.namedParameters))) {
	print_usage();
	process.exit(-1);
}

console.log ('Running prvbucketserver and mountainprocessserver...');

// Exit this program when the user presses ctrl+C
process.on('SIGINT', function() {
    process.exit();
});

// Create the web server!
var handler_opts={};
handler_opts.server_url=server_url;
handler_opts.prv_exe=prv_exe;
handler_opts.mp_exe=mp_exe;
handler_opts.prvbucket_url_path=prvbucket_url_path;
handler_opts.mountainprocess_url_path=mountainprocess_url_path;
handler_opts.data_directory=data_directory;
hopts=handler_opts;
var handler1=new prvbucketserver.RequestHandler(handler_opts);
var handler2=new mountainprocessserver.RequestHandler(handler_opts);
var SERVER=http.createServer(function(REQ,RESP) {
	//parse the url of the request
	var url_parts = url.parse(REQ.url,true);
	var host=url_parts.host;
	var path=url_parts.pathname;
	var query=url_parts.query;

	if (REQ.method == 'OPTIONS') {
		var headers = {};
		
		//allow cross-domain requests
		/// TODO refine this
		
		headers["Access-Control-Allow-Origin"] = "*";
		headers["Access-Control-Allow-Methods"] = "POST, GET, PUT, DELETE, OPTIONS";
		headers["Access-Control-Allow-Credentials"] = false;
		headers["Access-Control-Max-Age"] = '86400'; // 24 hours
		headers["Access-Control-Allow-Headers"] = "X-Requested-With, X-HTTP-Method-Override, Content-Type, Accept";
		RESP.writeHead(200, headers);
		RESP.end();
	}
	else {
		if (path.indexOf(hopts.prvbucket_url_path)===0) {
			handler1.handle_request(REQ,RESP);
		}
		else if (path.indexOf(hopts.mountainprocess_url_path)===0) {
			handler2.handle_request(REQ,RESP);
		}
		else {
			if (REQ.method=='GET') {
				send_json_response({success:false,error:'Unexpected path **: '+path});
				return;
			}
			else if(REQ.method=='POST') {
				send_text_response("Unsuported request method: POST");
			}
			else {
				send_text_response("Unsuported request method.");
			}
		}
	}

	function send_json_response(obj) {
		console.log ('RESPONSE: '+JSON.stringify(obj));
		RESP.writeHead(200, {"Access-Control-Allow-Origin":"*", "Content-Type":"application/json"});
		RESP.end(JSON.stringify(obj));
	}
	function send_text_response(text) {
		console.log ('RESPONSE: '+text);
		RESP.writeHead(200, {"Access-Control-Allow-Origin":"*", "Content-Type":"text/plain"});
		RESP.end(text);
	}
	function send_html_response(text) {
		console.log ('RESPONSE: '+text);
		RESP.writeHead(200, {"Access-Control-Allow-Origin":"*", "Content-Type":"text/html"});
		RESP.end(text);
	}
});
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