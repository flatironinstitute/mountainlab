#!/usr/bin/env nodejs

function print_usage() {
	console.log ('Usage:');
	console.log ('mpdock_update.mp spec');	
}

function print_spec() {
	var spec={
		processors:[
			{
				name:'mpdock.update',
				version:'0.1',
				inputs:[
				],
				outputs:[
				],
				parameters:[
					{name:'package_uri',optional:false}
				],
				opts:{
					force_run:true
				},
				exe_command:__dirname+'/mpdock update --package_uri=$package_uri$'
			}
		]
	};
	console.log (JSON.stringify(spec,null,4));
}

var CLP=new CLParams(process.argv);

var arg1=CLP.unnamedParameters[0]||'';
if (!arg1) {
	print_usage();
	process.exit(-1);
}

if (arg1=='spec') {
	print_spec();
}
else {
	console.error('Unexpected command: '+arg1);
	process.exit(-1);	
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
