from pathlib import Path
import subprocess

Import("env")

project_dir = Path(env.subst("$PROJECT_DIR"))
pioenv = env.subst("$PIOENV")
libcan_dir = project_dir / ".pio" / "libdeps" / pioenv / "LibCAN"

if libcan_dir.exists():
    git_dir = libcan_dir / ".git"
    if not git_dir.exists():
        print(f"LibCAN dependency exists but is not a Git checkout: {libcan_dir}")
    else:
        print(f"Refreshing LibCAN dependency: {libcan_dir}")
        subprocess.run(["git", "-C", str(libcan_dir), "fetch", "--prune", "origin"], check=True)
        branch = subprocess.check_output(
            ["git", "-C", str(libcan_dir), "rev-parse", "--abbrev-ref", "HEAD"],
            text=True,
        ).strip()
        if branch == "HEAD":
            branch = "main"
        subprocess.run(["git", "-C", str(libcan_dir), "reset", "--hard", f"origin/{branch}"], check=True)
