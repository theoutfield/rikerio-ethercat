let g:solarized_termcolors=256

syntax enable
set background=light

autocmd BufEnter * colorscheme solarized
autocmd BufEnter *.md colorscheme default

filetype plugin indent on
set tabstop=4
set shiftwidth=4
set expandtab

au BufWrite * :Autoformat

let g:autoformat_autoindent = 0
let g:autoformat_retab = 0
let g:autoformat_remove_trailing_spaces = 0

let g:sytastic_c_checkers = ['cmake']
let g:syntastic_always_populate_loc_list = 1
let g:syntastic_auto_loc_list = 1
let g:syntastic_check_on_open = 0
let g:syntastic_check_on_wq = 0

let g:syntastic_c_include_dirs = ['ethercat/include', 'lib/include', 'build', 'server/include', 'eventbus/include']
let g:syntastic_c_check_header = 0



