# from platformio.package.meta import PackageSpec
# Import('env')

# libs = [PackageSpec(lib).name for lib in env.GetProjectOption("lib_deps",[])]
# # Check for partner usermod
# # Allow both "usermod_v2" and unqualified syntax
# if any(mod in ("data_to_influxdb", "usermod_v2_data_to_influxdb") for mod in libs):
#     env.Append(CPPDEFINES=[("USERMOD_DATA_TO_INFLUXDB")])
