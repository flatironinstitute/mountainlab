#!/usr/bin/env nodejs

var config=read_json_file(__dirname+'/kulelepoller.user.json');
if (!config) {
	console.log ('Missing, empty, or invalid config file');
	return;
}

var kulele_url=config['kulele_url']||'';
var kulele_subserver_name=config['kulele_subserver_name']||'';
var kulele_subserver_passcode=config['kulele_subserver_passcode']||'';
var otherserver_url=config['otherserver_url'];

if ((!kulele_url)||(!kulele_subserver_name)||(!kulele_subserver_passcode)||(!otherserver_url)) {
	console.log ('Problem in config');
	return;
}

var show_logs=false;
function make_timestamp() {
	var dd=new Date();
	return dd.getHours()+':'+dd.getMinutes()+':'+dd.getSeconds()+':'+dd.getMilliseconds();
}
function log(str,msg) {
	if (show_logs) {
		var ts=make_timestamp();
		console.log ('===========================================================================');
		console.log ('========= '+ts+'    '+str);
		console.log (JSON.stringify(msg));
	}
}

// The "other" server is larinet
var H=new OtherServerHandler();
H.setOtherServerUrl(otherserver_url);

var KP=new KulelePoller();
KP.setKuleleUrl(kulele_url);
KP.setSubserverName(kulele_subserver_name);
KP.setSubserverPasscode(kulele_subserver_passcode);
KP.setRequestHandler(H);
KP.start();

function OtherServerHandler() {
	var that=this;
	this.setOtherServerUrl=function(url) {m_otherserver_url=url;};
	this.handleDownloadRequest=function(req,onclose,response_url,callback) {handleDownloadRequest(req,onclose,response_url,callback);};
	this.handleRequest=function(req,onclose,callback) {handleRequest(req,onclose,callback);};

	var m_otherserver_url='';
	
	function handleRequest(req,onclose,callback) {
		//send this request on to larinet (the "other" server)
		//if we are not in download we can just post to larinet and then post to kulele
		http_post_json(m_otherserver_url,req,onclose,function(tmp) {
			if (!tmp.success) {
				if (callback) callback(tmp);
				callback=0;
				return;
			}
			if (callback) callback(tmp.object);
			callback=0;
			/*
			http_post_json(response_url,tmp.object,onclose,function(tmp) {
				if (!tmp.success) {
					if (callback) callback(tmp);
					callback=0;
					return;
				}
				if (callback) callback(tmp.object);
				callback=0;
			});
			*/
		});
	}
	function handleDownloadRequest(req,onclose,response_url,callback) {
		//we are dealing with a client download
		if (!req.download_path) {
			if (callback) callback({success:false,error:'Expected req.download_path for download_mode=true'}); //goes back to kulele
			callback=0;
			return;
		}
		var ended=false;
		var num_bytes_posted=0;
		var path0=req.download_path;
		var get_opts={
			url:m_otherserver_url+'/'+path0+'?download_mode=true',
			method:'GET'
		}

		var holder={};

		var get_req = require('http').get(get_opts.url, function(res) {
			var headers_for_post={};
			if (res.headers['content-length'])
				headers_for_post['content-length']=res.headers['content-length'];
			headers_for_post['Content-Type']='application/octet-stream';
			if (req.download_path) {
				headers_for_post['content-disposition']="attachment; filename=\""+require('path').basename(req.download_path)+"\"";
			}
			start_post_request(headers_for_post);
			res.on('data', function (body) {
				holder.post_req.write(body);
			});
			res.on('end',function() {
				holder.post_req.end();
			});
		});

		function start_post_request(headers_for_post) {
			var url_parts00 = require('url').parse(response_url+'&download_mode=true',true);
			var host00=url_parts00.hostname;
			var path00=url_parts00.pathname;
			var port00=url_parts00.port;
			var query00=url_parts00.query;
			var post_opts={
				host: host00,
				port: port00,
				path: path00+'?'+require('querystring').stringify(query00),
				method: 'POST',
				headers: headers_for_post
			}

			holder.post_req=require('http').request(post_opts,function(res) {
				var body0='';
				res.on('data', function (body) {
					body0+=body;
					num_bytes_posted+=body.length;
				});
				res.on('end',function() {
					var obj;
					try {
						obj=JSON.parse(body0);
					}
					catch(err) {
						if (callback) callback({success:false,num_bytes_posted:num_bytes_posted,error:'Error parsing response from kulele'});
						callback=0;
						return;
					}
					obj.num_bytes_posted=num_bytes_posted;
					if (callback) callback(obj); //success!
					callback=0;
				});
			});
			holder.post_req.on('error',function(tmp) {
				get_req.connection.destroy();
				get_req.abort();
				if (callback) callback({success:false,error:'Problem in http post: '+response_url,num_bytes_posted:num_bytes_posted});
				callback=0;
			});	
		}
		
		get_req.on('error',function(tmp) {
			if (callback) callback({success:false,error:'Problem in http get: '+get_opts.url,num_bytes_posted:num_bytes_posted});
			callback=0;
		});
	}
}

function KulelePoller() {
	var that=this;
	this.setKuleleUrl=function(url) {m_kulele_url=url;}; //the url to kulele in the cloud, e.g. kulele.herokuapp.com
	this.setSubserverName=function(name) {m_subserver_name=name;}; //the name of this subserver, which must be registered in kulele in the cloud
	this.setSubserverPasscode=function(pc) {m_subserver_passcode=pc;}; //the passcode corresponding to this subserver
	this.setRequestHandler=function(h) {m_request_handler=h;}; //the arbitrary function that will handle the requests
	this.start=function() {start();}; //start the polling!

	var m_kulele_url='';
	var m_subserver_name='';
	var m_subserver_passcode='';
	var m_request_handler=function(REQ,RESP) {};
	var m_num_active_poll_requests=0; //the number of currently active poll requests
	var m_last_poll_request_timestamp=new Date(); //the last time we made a poll request
	var m_active_client_requests={}; //these are the active client requests that we are responding to

	//the following used to be 6, but on typhoon this caused a slowdown. using 1 is fine, i think
	var m_desired_num_active_poll_requests=1; //this is how many we want at any given time
	var m_poll_request_interval=20; //how long we wait (in milliseconds) until we send up another poll request
	var m_bundle_time=100;

	function start() {
		housekeeping(); //the housekeeping function does it all
	}

	var s_timer=new Date();
	function housekeeping() {
		var elapsed0=(new Date())-s_timer;
		if (elapsed0>5000) {
			print_status();
			s_timer=new Date();
		}
		make_poll_request_as_needed();
		setTimeout(housekeeping,10);
	}

	function print_status() {
		console.log ('num_active_poll_requests: '+m_num_active_poll_requests);
	}

	function make_poll_request_as_needed() {
		var elapsed=(new Date())-m_last_poll_request_timestamp;
		if ((m_num_active_poll_requests<m_desired_num_active_poll_requests)&&(elapsed>m_poll_request_interval)) {
			make_poll_request();
		}
	}
	function make_poll_request() {

		var url=m_kulele_url+'/from_subserver/'+m_subserver_name;
		var str='';
		str+='&subserver_passcode='+(m_subserver_passcode||'');
		var url0=url+'?request_type=poll'+str;
		m_num_active_poll_requests++; //increment
		m_last_poll_request_timestamp=new Date();
		//send the poll request
		get_json(url0,function(resp) {
			m_num_active_poll_requests--; //decrement
			if (!resp.success) {
				console.log ('Error in poll request: '+resp.error);
				return;
			}
			if (resp.used) { //the poll request was used for something!
				log('Received response from poll request',resp);
				if (resp.requests_from_client) { //there were some requests from the client(s)
					var reqs=resp.requests_from_client;
					for (var i in reqs) {
						handle_request_from_client(reqs[i]||{});
					}
				}
				else if (resp.cancel_client_request_id) {
					//kulele has requested to cancel this client request
					if (resp.cancel_client_request_id in m_active_client_requests) {
						m_active_client_requests[resp.cancel_client_request_id].to_run_on_close();
					}
				}
				else {
					console.log ('Warning: unexpected response to poll: '+JSON.stringify(resp));
				}
			}
		});
	}
	function handle_request_from_client(request_from_client) {
		console.log ('REQUEST FROM CLIENT: '+request_from_client.a);
		var url=m_kulele_url+'/from_subserver/'+m_subserver_name;
		var client_request_id=request_from_client.client_request_id; //it has an id that was assigned by kulele in the cloud
		if (!client_request_id) {
			console.log ('Unexpected problem: client_request_id='+client_request_id);
			return;
		}
		// here is the function we will run on close (see cancel_client_request_id below)
		var to_run_on_close0=function() {};
		var onclose=function(to_run_on_close) { //this function gets passed to the handler, so that we can end the handling if the request to kulele has been canceled 
			if (m_active_client_requests[client_request_id]) {
				var prev=m_active_client_requests[client_request_id].to_run_on_close;
				m_active_client_requests[client_request_id].to_run_on_close=function() {prev(); to_run_on_close();}; //this allows multiple functions to be called on close	
			}
		}
		m_active_client_requests[client_request_id]={to_run_on_close:to_run_on_close0}; //keep track of the active client requests
		if (request_from_client.download_mode=='true') {
			var url1=url+'?request_type=response_to_client_download&client_request_id='+client_request_id+'&subserver_passcode='+(m_subserver_passcode||'');
			m_request_handler.handleDownloadRequest(request_from_client,onclose,url1,function(tmp) { //handle the request. The handler will post to the url
				delete m_active_client_requests[client_request_id];
				if (!tmp.success) {
					console.log ('Error handling request from client (handler was supposed to post download to the url): '+tmp.error);
				}
			});
		}
		else {
			m_request_handler.handleRequest(request_from_client,onclose,function(resp) {
				delete m_active_client_requests[client_request_id];
				resp.client_request_id=client_request_id;
				schedule_send_response_to_client_request(resp);
			});
		}
	}
	var m_scheduled1=false;
	var m_responses_to_client_requests=[];
	function schedule_send_response_to_client_request(resp) {
		m_responses_to_client_requests.push(resp);
		if (m_scheduled1) return;
		m_scheduled1=true;
		setTimeout(function() {
			m_scheduled1=false;
			var responses=m_responses_to_client_requests;
			m_responses_to_client_requests=[];
			do_send_responses_to_client_requests(responses);
		},m_bundle_time);
	}
	function do_send_responses_to_client_requests(responses) {
		var url=m_kulele_url+'/from_subserver/'+m_subserver_name;
		var url1=url+'?request_type=responses_to_client_requests&subserver_passcode='+(m_subserver_passcode||'');		
		var data0={
			responses:responses
		};
		log('Posting response to client request',{url:url1,data:data0});
		http_post_json(url1,data0,null,function(tmp) {
			if (!tmp.success) {
				console.log ('Error sending responses to client requests: '+tmp.error);
			}
		});
	}
}

function get_json(url,callback) {
	http_get_json(url,function(tmp) {
		if (!tmp.success) {
			callback(tmp);
			return;
		}
		callback(tmp.object);
	})
}
function post_json(url,data,callback) {
	http_post_json(url,data,null,function(tmp) {
		if (!tmp.success) {
			callback(tmp);
			return;
		}
		callback(tmp.object);
	})
}

function http_get_json(url,callback) {
	http_get_text(url,function(tmp) {
		if (!tmp.success) {
			callback(tmp);
			return;
		}
		var obj;
		try {
			obj=JSON.parse(tmp.text);
		}
		catch(err) {
			callback({success:false,error:'Error parsing json response: '+tmp.text});
			return;
		}
		callback({success:true,object:obj});
	});
}

function http_get_text(url,callback) {
	// Set up the request

	var HTTP=require('http');
	if (starts_with(url,'https')) HTTP=require('https');

	var get_req = HTTP.get(url, function(res) {
		res.setEncoding('utf8');
		var chunks=[];
		res.on('data', function (body) {
			chunks.push(body);
		});
		res.on('end',function() {
			var text=chunks.join();
			callback({success:true,text:text});
		});
	});
	get_req.on('error',function() {
		callback({success:false,error:'Problem in http get: '+url});
	});
}

/*
function http_post_json(url,data,onclose,callback) {
	var url_parts = require('url').parse(url,true);
	var host=url_parts.hostname;
	var path=url_parts.pathname;
	var port=url_parts.port;

	var post_data=JSON.stringify(data);

	// An object of options to indicate where to post to
	var post_options = {
		host: host,
		port: port,
		path: path,
		method: 'POST',
		headers: {
		  'Content-Type': 'application/json',
		  'Content-Length': Buffer.byteLength(post_data)
		}
	};

	// Set up the request
	var post_req = require('http').request(post_options, function(res) {
		res.setEncoding('utf8');
		res.on('data', function (body) {
			try {
				var obj=JSON.parse(body);
				callback({success:true,object:obj});
			}
			catch(err) {
				callback({success:false,error:'Problem parsing json response: '+body});
			}
		});
	});
	post_req.on('error',function() {
		callback({success:false,error:'Error in post: '+url});
	});

	if (onclose) {
		onclose(function() {
			post_req.connection.destroy();
			post_req.abort();
		});
	}

	// post the data
	post_req.write(post_data);
	post_req.end();
}
*/

function http_post_json(url,data,onclose,callback) {
	var url_parts = require('url').parse(url,true);
	var host=url_parts.hostname;
	var path=url_parts.pathname;
	var port=url_parts.port;
	var query=url_parts.query;

	var post_data=JSON.stringify(data);

	// An object of options to indicate where to post to
	var post_options = {
		host: host,
		port: port,
		path: path+'?'+require('querystring').stringify(query),
		method: 'POST'
	};

	var HTTP=require('http');
	if (starts_with(url,'https')) HTTP=require('https');

	// Set up the request
	var post_req = HTTP.request(post_options, function(res) {
		res.setEncoding('utf8');
		var chunks=[];
		res.on('data', function (body) {
			chunks.push(body);
		});
		res.on('end',function() {
			var body=chunks.join();
			var obj;
			try {
				obj=JSON.parse(body);
			}
			catch(err) {
				callback({success:false,error:'Problem parsing json response: '+body});
				return;
			}
			callback({success:true,object:obj});
		})
	});
	post_req.on('error',function() {
		callback({success:false,error:'Error in post: '+url});
	});
	if (onclose) {
		onclose(function() {
			post_req.connection.destroy();
			post_req.abort();
		});
	}

	// post the data
	post_req.write(post_data);
	post_req.end();
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

function read_text_file(fname) {
	try {
		return require('fs').readFileSync(fname,'utf8');
	}
	catch(err) {
		console.log ('Problem reading text file: '+fname);
		return '';
	}
}

function read_json_file(fname) {
	var txt=read_text_file(fname);
	if (!txt) return null;
	try {
		return JSON.parse(txt);
	}
	catch(err) {
		console.log ('Error parsing json: '+txt);
		return null;
	}	
}

function starts_with(str,str2) {
	return (str.slice(0,str2.length)==str2);
}