exports.mpdock_update_package=mpdock_update_package;

var sha1=require(__dirname+'/sha1.js');

function mpdock_update_package(package_uri,opts,callback) {
	var package={
		git_uri:package_uri,
		name:get_package_name_from_git_uri(package_uri)
	};
	if (opts.build) {
		if (opts.verbose) {
			console.log (`Building package: ${package.name} ...`);
		}
		system_call('docker build -t mpdock/'+package.name+' '+package.git_uri,{print_console:opts.verbose||true},function(err,tmp) {
			if (err) {
				callback(`Error building docker image for package ${package.name}: `+err);
				return;
			}
			step2();
		});
	}
	else {
		step2();
	}
	function step2() {
		if (opts.verbose) {
			console.log ('Reading package spec from container.');
		}
		read_package_spec_from_container('mpdock/'+package.name,package.git_uri,package.prefix,function(err,spec) {
			if (err) {
				callback(`Error reading spec from container for package ${package.name}: `+err);
				return;
			}
			callback('',spec);
		});
	}
}

function get_package_name_from_git_uri(uri) {
	var uri2=uri;
	var ind1=uri2.lastIndexOf('#');
	if (ind1>=0) {
		uri2=uri2.slice(0,ind1);
	}
	var list=uri2.split('/');
	var name=list[list.length-1];
	var code=sha1(uri).slice(0,6);
	return name+'_'+code;
}

function read_package_spec_from_container(image_tag,package_uri,prefix,callback) {
	var aaa="echo '[' ; for i in *.spec; do cat \\$i; echo ','; done ; echo '{}]'";
	var cmd2='docker run -t '+image_tag+' /bin/bash -c "'+aaa+'"';
	system_call(cmd2,{print_console:false},function(err,tmp2) {
		if (err) {
			callback(err);
			return;
		}
		var processors=[];
		var json=tmp2.console_out;
		var specs=JSON.parse(json);
		for (var j=0; j<specs.length; j++) {
			if (specs[j].processors) {
				for (var k=0; k<specs[j].processors.length; k++) {
					var pp=specs[j].processors[k];
					if (prefix)
						pp.name=prefix+'.'+pp.name;
					pp=adjust_processor(pp,image_tag,package_uri);
					processors.push(pp);
				}
			}
		}
		callback('',{processors:processors});
	});
}

function adjust_processor(pp,image_tag,package_uri) {
	var pp2=JSON.parse(JSON.stringify(pp));
	pp2.docker_image_tag=image_tag;
	pp2.exe_command_within_container=pp.exe_command;
	pp2.exe_command=__dirname+'/mpdock exec '+pp.name+' $(arguments) --package_uri='+package_uri;
	pp2.package_uri=package_uri;
	return pp2;
}

function system_call(cmd,opts,callback) {
	if (opts.print_console)
		console.log ('RUNNING: '+cmd);

	var pp=require('child_process').exec(cmd);
	pp.stdout.setEncoding('utf8');
	pp.stderr.setEncoding('utf8');
	var console_out='';
	pp.on('close', function(code,data) {
		if (data) console_out+=data;
		if (callback) callback('',{console_out:console_out,exit_code:code});
		callback=0;
	});
	/*
	//important not to do the following, because we can miss the console output
	pp.on('exit', function(code,data) {
		if (data) console_out+=data;
		if (callback) callback({success:true,console_out:console_out,exit_code:code});
		callback=0;
	});
	*/
	pp.on('error',function(err) {
		if (callback) callback(err.message,{console_out:console_out,exit_code:-1});
		callback=0;
	});
	pp.stdout.on('data',function(data) {
		if (opts.print_console)
			console.log (data.trim());
		console_out+=data;
	});
	pp.stderr.on('data',function(data) {
		if (opts.print_console)
			console.log (data.trim());
		console_out+=data;
	});
};