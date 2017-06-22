let g:solarized_termcolors=256

syntax enable
set background=light
"colorscheme solarized
colorscheme gruvbox

Plugin 'digitalrounin/vim-yaml-folds'

filetype plugin indent on
set tabstop=4
set shiftwidth=4
set expandtab

au BufWrite * :Autoformat

set statusline+=%#warningmsg#
set statusline+=%{SyntasticStatuslineFlag()}
set statusline+=%*

let g:autoformat_autoindent = 0
let g:autoformat_retab = 0
let g:autoformat_remove_trailing_spaces = 0

let g:syntastic_c_checkers = ['cmake']
let g:syntastic_always_populate_loc_list = 1
let g:syntastic_auto_loc_list = 1
let g:syntastic_check_on_open = 0
let g:syntastic_check_on_wq = 0

let g:syntastic_c_include_dirs = ['include', 'build', '/usr/include']
let g:syntastic_c_check_header = 0


