# Flujo inicial de Vitis HLS para el kernel RGB -> gris.
#
# Uso:
#   vitis_hls -f hls/scripts/run_hls.tcl
#
# Variables opcionales de entorno:
#   HLS_PROJECT_DIR : ruta base donde se crea el proyecto HLS
#   HLS_PART        : particion/target para Vitis HLS (por defecto xczu3eg-sbva484-1-i)
#   HLS_CLOCK_NS    : periodo del reloj en ns (por defecto 4)

set script_dir [file normalize [file dirname [info script]]]
set project_dir [expr {[info exists ::env(HLS_PROJECT_DIR)] ? [file normalize $::env(HLS_PROJECT_DIR)] : [file normalize [file join $script_dir ..]]}]
set src_dir [file join $project_dir src]
set tb_dir [file join $project_dir tb]
set reports_dir [file join $project_dir reports]
set build_dir [file join $project_dir build img_accel_hls]
set log_dir [file join $reports_dir logs]

set part [expr {[info exists ::env(HLS_PART)] ? $::env(HLS_PART) : "xczu3eg-sbva484-1-i"}]
set clock_period [expr {[info exists ::env(HLS_CLOCK_NS)] ? $::env(HLS_CLOCK_NS) : 4}]

file mkdir $build_dir
file mkdir $reports_dir
file mkdir $log_dir

puts "[HLS] proyecto: $build_dir"
puts "[HLS] reportes: $reports_dir"
puts "[HLS] particion: $part"
puts "[HLS] periodo: $clock_period ns"

if {[file exists $build_dir]} {
    open_project -reset $build_dir
} else {
    open_project $build_dir
}

set_top rgb_to_gray_top
add_files [file join $src_dir img_accel_hls.cpp]
add_files -tb [file join $tb_dir img_accel_tb.cpp]

set_part $part
create_clock -period $clock_period -name default

csim_design
synth_design
cosim_design
export_design -format ip_catalog

close_project
puts "Flujo HLS finalizado. Reportes en $reports_dir"
