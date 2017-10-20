function mp_queue_process(processor_name,inputs,outputs,params,opts)

if nargin<4, params=struct; end;
if nargin<5, opts=struct; end;
opts.mp_command='mp-queue-process';

mp_run_process(processor_name,inputs,outputs,params,opts);
