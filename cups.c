#include <cups/cups.h>
#include <ruby/ruby.h>

#ifndef MAXOPTS
  #define MAXOPTS 100
#endif

static int id_push;
static int num_options;
static cups_option_t *options;
cups_dest_t *dests, *dest;

static VALUE job_init(VALUE self, VALUE filename, VALUE printer) {
  rb_iv_set(self, "@filename", filename);
  rb_iv_set(self, "@printer", printer);
  return self;
}

// Submit a print job to the selected printer or class
static VALUE cups_print(VALUE self, VALUE file, VALUE printer) {
  int job_id;
  file = rb_iv_get(self, "@filename");
  printer = rb_iv_get(self, "@printer");
  
  char *fname = RSTRING_PTR(file); // Filename
  char *target = RSTRING_PTR(printer); // Target printer string
  
  FILE *fp = fopen(fname,"r");
  // Check @filename actually exists...
  if( fp ) {
    fclose(fp);
    job_id = cupsPrintFile(target, fname, "rCUPS", num_options, options); // Do it.
    rb_iv_set(self, "@job_id", INT2NUM(job_id));
    return job_id;
  } else {
  // and if it doesn't...
    rb_raise(rb_eRuntimeError, "Couldn't find file");
    return self;
  }
}

static VALUE cups_show_dests(VALUE self, VALUE dest_list) {
  int i;
  int num_dests = cupsGetDests(&dests); // Size of dest_list array
  dest_list = rb_ary_new();
  
  for (i = num_dests, dest = dests; i > 0; i --, dest ++) {
    VALUE destination = rb_str_new2(dest->name);
    rb_ary_push(dest_list, destination); // Add this testination name to dest_list string
  }
  return dest_list;
}

// Get default printer or class. Returns a string or false if there is no default
static VALUE cups_get_default(VALUE self) {
  const char *default_printer;
  default_printer = cupsGetDefault();

  if (default_printer != NULL) {
    VALUE def_p = rb_str_new2(default_printer);
    return def_p;
  }
}

// Cancel the current job. Returns true if successful, false otherwise.
static VALUE cups_cancel(VALUE self) {
  VALUE printer, job_id;
  printer = rb_iv_get(self, "@printer");
  job_id = rb_iv_get(self, "@job_id");
  
  if (NIL_P(job_id)) {
    return Qfalse; // If @job_id is nil
  } else { // Otherwise attempt to cancel
    int job = NUM2INT(job_id);
    char *target = RSTRING_PTR(printer); // Target printer string
    int cancellation;
    cancellation = cupsCancelJob(target, job);
    return Qtrue;
  }
}

// Convenience method for CUPS job id. Returns nil if job hasn't been submitted.
static VALUE cups_job_id(VALUE self) {
  VALUE job_id = rb_iv_get(self, "@job_id");
  return self;
}

// Get jobs
static VALUE cups_get_jobs(VALUE self) {
  VALUE job_list;
  int i;
  cups_job_t *jobs;
  // int num_jobs = cupsGetJobs(job, NULL, 1, 1); // Get jobs
  job_list = rb_ary_new();
  
  // for (i = num_jobs, job = jobs; i > 0; i --, job ++) {
  //   VALUE destination = rb_str_new2(dest->name);
  //   rb_ary_push(job_list, destination); // Add this testination name to dest_list string
  // }
  return job_list;
}

VALUE printJobs;

void Init_cups() {
  printJobs = rb_define_class("PrintJob", rb_cObject);
  rb_define_method(printJobs, "initialize", job_init, 2);
  rb_define_method(printJobs, "print", cups_print, 0);
  rb_define_method(printJobs, "cancel", cups_cancel, 0);
  rb_define_method(printJobs, "job_id", cups_job_id, 0);
  rb_define_singleton_method(printJobs, "show_destinations", cups_show_dests, 0);
  rb_define_singleton_method(printJobs, "default_printer", cups_get_default, 0);
  rb_define_singleton_method(printJobs, "all_jobs", cups_get_jobs, 0);
  id_push = rb_intern("push");
}