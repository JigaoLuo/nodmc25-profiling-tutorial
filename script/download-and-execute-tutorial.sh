PROJECT_FOLDER_NAME=nodmc25-profiling-tutorial

# Remove the folder if it exists
rm -rf $PROJECT_FOLDER_NAME

# Checkout the repo
git clone git@github.com:jmuehlig/nodmc25-tutorial-internal.git $PROJECT_FOLDER_NAME

# Execute
cd $PROJECT_FOLDER_NAME
sh script/execute-tutorial.sh