from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile

try:
    Import("env")
except NameError:
    env = None

project_dir = (Path(env.subst("$PROJECT_DIR")) if env is not None
               else Path(__file__).resolve().parent.parent)
output_path_sdk = project_dir / "docs" / "PRUZEAmini_SDK.zip"
output_path_project = project_dir / "docs" / "PRUZEAmini_PROJECT.zip"
input_files_sdk = (
    (project_dir / "README.md", "README.md"),
    (project_dir / "src" / "PRUZEAmini.h", "PRUZEAmini.h"),
    (project_dir / "docs" / "PRUZEAmini_AI_GUIDELINES.md", "PRUZEAmini_AI_GUIDELINES.md"),

    (project_dir / "examples" / "00B_Hardware_Setup" / "00B_Hardware_Setup.ino", "examples/00B_Hardware_Setup/00B_Hardware_Setup.ino"),
    (project_dir / "examples" / "01_Hello_PRUZEA" / "01_Hello_PRUZEA.ino", "examples/01_Hello_PRUZEA/01_Hello_PRUZEA.ino"),
    (project_dir / "examples" / "02_Input_Basics" / "02_Input_Basics.ino", "examples/02_Input_Basics/02_Input_Basics.ino"),
    (project_dir / "examples" / "03_Graphics_Basics" / "03_Graphics_Basics.ino", "examples/03_Graphics_Basics/03_Graphics_Basics.ino"),
    (project_dir / "examples" / "04_Audio_Basics" / "04_Audio_Basics.ino", "examples/04_Audio_Basics/04_Audio_Basics.ino"),
    (project_dir / "examples" / "05_Save_Data" / "05_Save_Data.ino", "examples/05_Save_Data/05_Save_Data.ino"),
)
output_path_sdk.parent.mkdir(parents=True, exist_ok=True)
with ZipFile(output_path_sdk, "w", compression=ZIP_DEFLATED) as archive:
    for source_path, archive_name in input_files_sdk:
        if not source_path.is_file():
            raise RuntimeError(f"SDK source file not found: {source_path}")
        archive.write(source_path, archive_name)

input_files_project = (
    (project_dir / "docs" / "PRUZEAmini_GAME_DESIGN_TEMPLATE.md", "PRUZEAmini_GAME_DESIGN_TEMPLATE.md"),
    (project_dir / "docs" / "PRUZEAmini_GAME_DESIGN_TEMPLATE_JP.md", "PRUZEAmini_GAME_DESIGN_TEMPLATE_JP.md"),
    (project_dir / "docs" / "PRUZEAmini_HARDWARE_CONFIG_TEMPLATE.md", "PRUZEAmini_HARDWARE_CONFIG_TEMPLATE.md"),
)
with ZipFile(output_path_project, "w", compression=ZIP_DEFLATED) as archive:
    for source_path, archive_name in input_files_project:
        if not source_path.is_file():
            raise RuntimeError(f"SDK source file not found: {source_path}")
        archive.write(source_path, archive_name)


print(f"PRUZEA mini SDK: {output_path_sdk}")
print(f"PRUZEA mini project files: {output_path_project}")
