PROJECT_FOLDER_NAME=nodmc25-profiling-tutorial

# Remove the folder if it exists
rm -rf $PROJECT_FOLDER_NAME

# Checkout the repo
git clone https://github.com/jmuehlig/nodmc25-profiling-tutorial.git $PROJECT_FOLDER_NAME

# Execute
cd $PROJECT_FOLDER_NAME
sh script/execute-tutorial.sh