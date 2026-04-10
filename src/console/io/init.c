void init_io(void) {
        void init_poll(void);
        void init_input(void);
        void init_io_output(void);
        void init_clipboard(void);

        init_poll();
        init_input();
        init_io_output();
        init_clipboard();
}
