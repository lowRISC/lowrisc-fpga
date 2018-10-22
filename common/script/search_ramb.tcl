# See LICENSE.Cambridge for license details.
# script to search for Block RAMs to be updated with new bare metal software

set origin_dir "."
set project_name [lindex $argv 0]
set orig_proj_dir [file normalize $origin_dir/$project_name]

# open project
open_project $orig_proj_dir/$project_name.xpr

# open implemented design
open_run impl_1

# search for all RAMB blocks
foreach m [get_cells ram_reg_*] { put $m; report_property $m {LOC} }
#report_property  [get_cells ram_reg_1] {LOC}
#report_property  [get_cells ram_reg_2] {LOC}
#report_property  [get_cells ram_reg_3] {LOC}
#report_property  [get_cells ram_reg_4] {LOC}
#report_property  [get_cells ram_reg_5] {LOC}
#report_property  [get_cells ram_reg_6] {LOC}
#report_property  [get_cells ram_reg_7] {LOC}
