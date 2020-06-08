def promotionConfig = [
    // Mandatory parameters
    'targetRepo'         : 'fpga-deb-release',

    // Optional parameters

    // The build name and build number to promote. If not specified, the Jenkins job's build name and build number are used
    'buildName'          : 'urjtag',
    'buildNumber'        : env.BUILD_NUMBER,
    // Comment and Status to be displayed in the Build History tab in Artifactory
    'comment'            : 'this is the promotion comment',
    'status'             : 'Released',
    // Specifies the source repository for build artifacts.
    'sourceRepo'         : 'fpga-deb-nightly',
    // Indicates whether to promote the build dependencies, in addition to the artifacts. False by default
    //'includeDependencies': true,
    // Indicates whether to copy the files. Move is the default
    'copy'               : true,
    // Indicates whether to fail the promotion process in case of failing to move or copy one of the files. False by default.
    'failFast'           : true
]

node('merlin')
{
  stage('Checkout')
  {
    cleanWs()
    checkout scm
  }
  stage('Build x86_64')
  {
    sh "build=x86_64 BUILD_NUMBER=${env.BUILD_NUMBER} .ci/jenkins_build.sh"
  }
  stage('Build armv7l')
  {
    sh "build=armv7l BUILD_NUMBER=${env.BUILD_NUMBER} .ci/jenkins_build.sh"
  }
  stage('Archive')
  {
    archiveArtifacts artifacts: 'urjtag_*.*.*-*_*.deb', fingerprint: true
  }
  if(env.BRANCH_NAME == "master")
  {
    stage('Upload')
    {
      def uploadSpec = """{
        "files": [
          {
            "pattern": "urjtag_*.*.*-*_amd64.deb",
            "target": "fpga-deb-nightly/pool/urjtag/",
            "props": "deb.distribution=xenial;deb.component=contrib;deb.architecture=amd64"
          }.
          {
            "pattern": "urjtag_*.*.*-*_armhf.deb",
            "target": "fpga-deb-nightly/pool/urjtag/",
            "props": "deb.distribution=xenial;deb.component=contrib;deb.architecture=armhf"
          }
        ]
      }"""
      def server = Artifactory.server 'artifactory'
      def buildInfo = Artifactory.newBuildInfo()
      buildInfo.env.capture = true
      buildInfo.env.collect()
      buildInfo.name = 'urjtag'
      buildInfo.retention maxBuilds: 10, deleteBuildArtifacts: true, async: true
      server.upload spec: uploadSpec, buildInfo: buildInfo
      server.publishBuildInfo buildInfo

      Artifactory.addInteractivePromotion server: server, promotionConfig: promotionConfig, displayName: "Promote me please"
    }
  }
}
