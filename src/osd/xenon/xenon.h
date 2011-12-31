// init libxenon
int xenon_init();
// update video
void xenon_update_video(render_primitive_list &primlist);
void xenon_set_dim(int w,int h);

// input
void xenon_input_init(running_machine &machine);
void xenon_update_input();