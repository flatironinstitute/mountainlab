var exports = module.exports = {};

//// requires
var	url=require('url');
var fs=require('fs');

exports.RequestHandler=function(hopts) {
	this.handle_request=function(REQ,RESP) {
		//parse the url of the request
		var url_parts = url.parse(REQ.url,true);
		var host=url_parts.host;
		var path=url_parts.pathname;
		var query=url_parts.query;
		
		var action=query.a||'';	

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
		else if (REQ.method=='GET') {
			console.log ('GET: '+REQ.url);

			if (path.indexOf(hopts.mountainprocess_url_path)===0) {
				path=path.slice(hopts.mountainprocess_url_path.length); // now let's take everything after that path
				if (action=="mountainprocess") {
					var request_fname=make_tmp_json_file(hopts.data_directory);
					var response_fname=make_tmp_json_file(hopts.data_directory);
					if (!write_text_file(request_fname,query.mpreq)) {
						send_json_response({success:false,error:'Problem writing mpreq to file'});
						return;
					}
					var done=false;
					var ppp=run_process_and_read_stdout(hopts.mp_exe,['handle-request','--prvbucket_path='+hopts.data_directory,request_fname,response_fname],function(txt) {
						done=true;
						//remove_file(request_fname);
						if (!fs.existsSync(response_fname)) {
							send_json_response({success:false,error:'Response file does not exist: '+response_fname});
							return;
						}
						var json_response=read_json_file(response_fname);
						remove_file(response_fname);
						if (json_response) {
							send_json_response(json_response);
						}
						else {
							send_json_response({success:false,error:'unable to parse json in response_fname'});
						}
					});
					REQ.on('close', function (err) {
						if (!done) {
							//request has been canceled
							if (ppp) {
								console.log( 'Terminating process because request has been canceled')
								console.log(ppp.pid);
								ppp.stdout.pause();
								ppp.kill('SIGKILL');
							}
						}
					});
				}
				else {
					send_json_response({success:false,error:"Unrecognized action: "+action});
				}
			}
			else {
				send_json_response({success:false,error:'Unexpected path: '+path});
				return;
			}
			
		}
		else if(REQ.method=='POST') {
			send_text_response("Unsuported request method: POST");
		}
		else {
			send_text_response("Unsuported request method.");
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
	};
};

function run_process_and_read_stdout(exe,args,callback) {
	console.log ('RUNNING:'+exe+' '+args.join(' '));
	var P;
	try {
		P=require('child_process').spawn(exe,args);
	}
	catch(err) {
		console.log (err);
		console.log ("Problem launching: "+exe+" "+args.join(" "));
		return 0;
	}
	var txt='';
	P.stdout.on('data',function(chunk) {
		txt+=chunk;
	});
	P.on('close',function(code) {
		callback(txt);
	});
	return P;
}

function write_text_file(fname,txt) {
	try {
		fs.writeFileSync(fname,txt);
		return true;
	}
	catch(err) {
		return false;
	}
}

function read_text_file(fname) {
	try {
		return fs.readFileSync(fname,'utf8');
	}
	catch(err) {
		return '';
	}
}

function read_json_file(fname) {
	try {
		return JSON.parse(read_text_file(fname));
	}
	catch(err) {
		return 0;
	}	
}

function make_tmp_json_file(data_directory) {
	return data_directory+'/tmp.'+make_random_id(10)+'.json';
}

function make_random_id(len)
{
    var text = "";
    var possible = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

    for( var i=0; i < len; i++ )
        text += possible.charAt(Math.floor(Math.random() * possible.length));

    return text;
}

function remove_file(fname) {
	try {
		fs.unlinkSync(fname);
		return true;
	}
	catch(err) {
		return false;
	}
}

function starts_with(str,substr) {
	return (str.slice(0,substr.length)==substr);
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
