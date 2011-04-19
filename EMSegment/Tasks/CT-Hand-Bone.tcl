package require Itcl

#########################################################
#
if {0} { ;# comment

    This is function is executed by EMSegmenter

    # TODO :

}
#
#########################################################

#
# namespace procs
#

#
# Remember to source first GenericTask.tcl as it has all the variables/basic structure defined
#
namespace eval EMSegmenterPreProcessingTcl {

    #
    # Variables Specific to this Preprocessing
    #
    variable TextLabelSize 1
    variable CheckButtonSize 3
    variable VolumeMenuButtonSize 0
    variable TextEntrySize 0

    # Check Button
    variable atlasAlignedFlagID 0
    variable rightHandFlagID 1
    variable inhomogeneityCorrectionFlagID 2

    # Text Entry
    # not defined for this task

    #
    # OVERWRITE DEFAULT
    #

    # -------------------------------------
    # Define GUI
    # return 1 when error occurs
    # -------------------------------------
    proc ShowUserInterface { } {
        variable preGUI
        variable atlasAlignedFlagID
        variable rightHandFlagID
        variable inhomogeneityCorrectionFlagID
        variable LOGIC

        # Always has to be done initially so that variables are correctly defined
        if { [InitVariables] } {
            PrintError "ERROR: MRI-HumanBrain: ShowUserInterface: Not all variables are correctly defined!"
            return 1
        }
        $LOGIC PrintText "TCLMRI: Preprocessing MRI Human Brain - ShowUserInterface"

        $preGUI DefineTextLabel "This task only applies to right handed scans scans! \n\nShould the EMSegmenter " 0
        $preGUI DefineCheckButton "- register the atlas to the input scan ?" 0 $atlasAlignedFlagID
        $preGUI DefineCheckButton "- right hand scan?" 0 $rightHandFlagID
        $preGUI DefineCheckButton "- perform image inhomogeneity correction on input scan ?" 0 $inhomogeneityCorrectionFlagID

        # Define this at the end of the function so that values are set by corresponding MRML node
        $preGUI SetButtonsFromMRML
    }

    # -------------------------------------
    # Define Preprocessing Pipeline
    # return 1 when error occurs
    # -------------------------------------
    proc Run { } {
        variable preGUI
        variable workingDN
        variable alignedTargetNode
        variable inputAtlasNode
        variable mrmlManager
        variable LOGIC

        variable atlasAlignedFlagID
        variable rightHandFlagID
        variable inhomogeneityCorrectionFlagID

        $LOGIC PrintText "TCLMRI: =========================================="
        $LOGIC PrintText "TCLMRI: == Preprocress Data"
        $LOGIC PrintText "TCLMRI: =========================================="
        # ---------------------------------------
        # Step 1 : Initialize/Check Input
        if {[InitPreProcessing]} {
            return 1
        }

        set atlasAlignedFlag [ GetCheckButtonValueFromMRML $atlasAlignedFlagID ]
        set rightHandFlag [ GetCheckButtonValueFromMRML $rightHandFlagID ]
        set inhomogeneityCorrectionFlag [ GetCheckButtonValueFromMRML $inhomogeneityCorrectionFlagID ]

        set inputTargetNode [$mrmlManager GetTargetInputNode]
        set alignedTargetNode [$workingDN GetAlignedTargetNode]
        set inputAtlasNode [$mrmlManager GetAtlasInputNode]
        set alignedAtlasNode [$mrmlManager GetAtlasAlignedNode]

        if { $inputTargetNode != "" } {
            $LOGIC PrintText "Detected [$inputTargetNode GetNumberOfVolumes] inputTargetNodeVolumes"
        }
        if { $alignedTargetNode != "" } {
            $LOGIC PrintText "Detected [$alignedTargetNode GetNumberOfVolumes] alignedTargetNodeVolumes"
        }
        if { $inputAtlasNode != "" } {
            $LOGIC PrintText "Detected [$inputAtlasNode GetNumberOfVolumes] inputAtlasNodeVolumes"
        }
        if { $alignedAtlasNode != "" } {
            $LOGIC PrintText "Detected [$alignedAtlasNode GetNumberOfVolumes] alignedAtlasNodeVolumes"
        }


        if { $alignedAtlasNode == "" } {
            $LOGIC PrintText "TCL: Aligned Atlas was empty"
            set alignedAtlasNode [ $mrmlManager CloneAtlasNode $inputAtlasNode "Aligned"]
            $workingDN SetReferenceAlignedAtlasNodeID [$alignedAtlasNode GetID]
        } else {
            $LOGIC PrintText "TCL: Atlas was just synchronized"
            $mrmlManager SynchronizeAtlasNode $inputAtlasNode $alignedAtlasNode "Aligned"
        }


        if { $atlasAlignedFlag } {
            # (flip), fiducial_threshold, blur, binarize, choose largest component of: atlas and target
            # RegisterHandAtlas

            set inputTargetVolumeNode [$inputTargetNode GetNthVolumeNode 0]
            set inputTargetVolumeFileName [WriteDataToTemporaryDir $inputTargetVolumeNode Volume]

            if { $rightHandFlag } {
                set flip_is_necessary_for_target 0
                set flip_is_necessary_for_atlas 0
            } else {
                set flip_is_necessary_for_target 0
                set flip_is_necessary_for_atlas 1
            }




            set atlasRegistrationVolumeIndex -1;
            if {[[$mrmlManager GetGlobalParametersNode] GetRegistrationAtlasVolumeKey] != "" } {
                set atlasRegistrationVolumeKey [[$mrmlManager GetGlobalParametersNode] GetRegistrationAtlasVolumeKey]
                set atlasRegistrationVolumeIndex [$inputAtlasNode GetIndexByKey $atlasRegistrationVolumeKey]
            }

            if { $atlasRegistrationVolumeIndex < 0 } {
                PrintError "RegisterAtlas: Attempt to register atlas image but no atlas image selected!"
                return 1
            }

            set inputAtlasVolumeNode [$inputAtlasNode GetNthVolumeNode $atlasRegistrationVolumeIndex]
            set inputAtlasVolumeFileName [WriteDataToTemporaryDir $inputAtlasVolumeNode Volume]



            set blurredInputTargetVolumeFileName [CreateTemporaryFileNameForNode $inputTargetVolumeNode]
            CTHandBonePipeline $inputTargetVolumeFileName $blurredInputTargetVolumeFileName $flip_is_necessary_for_target

            set blurredInputAtlasVolumeFileName [CreateTemporaryFileNameForNode $inputAtlasVolumeNode]
            CTHandBonePipeline $inputAtlasVolumeFileName $blurredInputAtlasVolumeFileName $flip_is_necessary_for_atlas


            ### Call Brainsfit ###

            #        set transformfile RegisterAtlasToSubject { $outputAtlasFileName $outputFileName }

            set PLUGINS_DIR "[$::slicer3::Application GetPluginsDir]"
            set CMD "${PLUGINS_DIR}/BRAINSFit"

            set fixedVolumeFileName $blurredInputTargetVolumeFileName
            set CMD "$CMD --fixedVolume $fixedVolumeFileName"

            set movingVolumeFileName $blurredInputAtlasVolumeFileName
            set CMD "$CMD --movingVolume $movingVolumeFileName"

            set outputVolumeFileName [CreateFileName "Volume"]
            if { $outputVolumeFileName == "" } {
                PrintError "Failed to create a temporary file"
            }
            set CMD "$CMD --outputVolume $outputVolumeFileName"

            set linearTransform [CreateFileName "LinearTransform"]
            if { $linearTransform == "" } {
                PrintError "Failed to create a temporary file"
            }
            set CMD "$CMD --outputTransform $linearTransform"

            set CMD "$CMD --initializeTransformMode useMomentsAlign --transformType Rigid,Affine"

            $LOGIC PrintText "TCL: Executing $CMD"
            catch { eval exec $CMD } errmsg
            $LOGIC PrintText "TCL: $errmsg"



            ### Call BRAINSDemonWarp ###

            set CMD "${PLUGINS_DIR}/BRAINSDemonWarp"
            set CMD "$CMD -m $movingVolumeFileName -f $fixedVolumeFileName"
            set CMD "$CMD --initializeWithTransform $linearTransform"
            set oArgument [CreateFileName "Volume"]
            if { $oArgument == "" } {
                PrintError "Failed to create a temporary file"
            }
            set deformationfield [CreateFileName "Volume"]
            if { $deformationfield == "" } {
                PrintError "Failed to create a temporary file"
            }
            set CMD "$CMD -o $oArgument -O $deformationfield"
            #set CMD "$CMD -i 1000,500,250,125,60 -n 5 -e --numberOfMatchPoints 16"
            # fast - for debugging
            set CMD "$CMD -i 2,2,2,2,1 -n 5 -e --numberOfMatchPoints 16"

            $LOGIC PrintText "TCL: Executing $CMD"
            catch { eval exec $CMD } errmsg
            $LOGIC PrintText "TCL: $errmsg"


            #TODO: check here for return code

            ### Call Resample ###

            set fixedTargetChannel 0
            set fixedTargetVolumeNode [$alignedTargetNode GetNthVolumeNode $fixedTargetChannel]
            set fixedTargetVolumeFileName [WriteImageDataToTemporaryDir $fixedTargetVolumeNode]

            for { set i 0 } { $i < [$alignedAtlasNode GetNumberOfVolumes] } { incr i } {
                #            if { $i == $atlasRegistrationVolumeIndex} { continue }
                $LOGIC PrintText "TCL: Resampling atlas image $i ..."
                set inputAtlasVolumeNode [$inputAtlasNode GetNthVolumeNode $i]
                set outputAtlasVolumeNode [$alignedAtlasNode GetNthVolumeNode $i]
                set backgroundLevel [$LOGIC GuessRegistrationBackgroundLevel $inputAtlasVolumeNode]
                $LOGIC PrintText "TCL: Guessed background level: $backgroundLevel"

                set inputAtlasVolumeFileName [WriteImageDataToTemporaryDir $inputAtlasVolumeNode]
                set outputAtlasVolumeFileName [WriteImageDataToTemporaryDir $outputAtlasVolumeNode]

                set CMD "${PLUGINS_DIR}/BRAINSResample"
                set CMD "$CMD --inputVolume $inputAtlasVolumeFileName  --referenceVolume $fixedTargetVolumeFileName"
                set CMD "$CMD --outputVolume $outputAtlasVolumeFileName --deformationVolume $deformationfield"

                set CMD "$CMD --pixelType"
                set fixedTargetVolume [$fixedTargetVolumeNode GetImageData]
                set scalarType [$fixedTargetVolume GetScalarTypeAsString]
                switch -exact "$scalarType" {
                    "bit" { set CMD "$CMD binary" }
                    "unsigned char" { set CMD "$CMD uchar" }
                    "unsigned short" { set CMD "$CMD ushort" }
                    "unsigned int" { set CMD "$CMD uint" }
                    "short" -
                    "int" -
                    "float" { set CMD "$CMD $scalarType" }
                    default {
                        PrintError "BRAINSResample: cannot resample a volume of type $scalarType"
                        return 1
                    }
                }


                $LOGIC PrintText "TCL: Executing $CMD"
                catch { eval exec $CMD } errmsg
                $LOGIC PrintText "TCL: $errmsg"

                ReadDataFromDisk $outputAtlasVolumeNode $outputAtlasVolumeFileName Volume
                file delete -force $outputAtlasVolumeFileName
                
            }
            #end for loop
        }
        # end  atlas alignment
        

        # Status: At this point our atlas is aligned to the input data


        #       ComputeIntensityDistributions


        #        for { set i 0 } { $i < [$alignedTargetNode GetNumberOfVolumes] } {incr i} {
        #             $LOGIC PrintText "read $i th alignedTargetNode"
        #            set intputVolumeNode($i) [$inputTargetNode GetNthVolumeNode $i]
        #            if { $intputVolumeNode($i) == "" } {
        #                PrintError "RegisterInputImages: the ${i}th input node is not defined!"
        #                return 1
        #            }
        #        }


        # ----------------------------------------------------------------------------
        # We have to create this function so that we can run it in command line mode
        #

        $LOGIC PrintText "TCLMRI: ==> Preprocessing Setting: $atlasAlignedFlag $inhomogeneityCorrectionFlag"


        # -------------------------------------
        # Step 4: Perform Intensity Correction
                if { $inhomogeneityCorrectionFlag == 1 } {
                    set dummy 0
                    set targetIntensityCorrectedCollectionNode [PerformIntensityCorrection $dummy]
                    if { $targetIntensityCorrectedCollectionNode == "" } {
                        PrintError "Run: Intensity Correction failed !"
                        return 1
                    }
                    if { [UpdateVolumeCollectionNode "$alignedTargetNode" "$targetIntensityCorrectedCollectionNode"] } {
                        return 1
                    }
                } else {
                    $LOGIC PrintText "TCLMRI: Skipping intensity correction"
                }

        # write results over to alignedTargetNode

        # -------------------------------------
        # Step 5: Atlas Alignment - you will also have to include the masks
        # Defines $workingDN GetAlignedAtlasNode
#        if { [RegisterAtlas $atlasAlignedFlag] } {
#            PrintError "Run: Atlas alignment failed !"
#            return 1
#        }


        # -------------------------------------
        # Step 6: Perform autosampling to define intensity distribution
        if { [ComputeIntensityDistributions] } {
            PrintError "Run: Could not automatically compute intensity distribution !"
            return 1
        }

        # -------------------------------------
        # Step 7: Check validity of Distributions
        set failedIDList [CheckAndCorrectTreeCovarianceMatrix]
        if { $failedIDList != "" } {
            set MSG "Log Covariance matrices for the following classes seemed incorrect:\n "
            foreach ID $failedIDList {
                set MSG "${MSG}[$mrmlManager GetTreeNodeName $ID]\n"
            }
            set MSG "${MSG}This can cause failure of the automatic segmentation. To address the issue, please visit the web site listed under Help"
            $preGUI PopUpWarningWindow "$MSG"
        }
        return 0
    }

    #
    # TASK SPECIFIC FUNCTIONS
    #
    proc CTHandBonePipeline { inputFileName outputFileName flip_is_necessary } {
        variable LOGIC

        set CTHandBoneHelper [vtkCTHandBoneClass New]

        if { $flip_is_necessary } {
            set TargetFlipFileName [CreateFileName "Volume"]
            if { $TargetFlipFileName == "" } {
                PrintError "Failed to create a temporary file"
            }
        }

        set TargetFlipThresholdFileName [CreateFileName "Volume"]
        if { $TargetFlipThresholdFileName == "" } {
            PrintError "Failed to create a temporary file"
        }
        set TargetFlipThresholdBlurFileName [CreateFileName "Volume"]
        if { $TargetFlipThresholdBlurFileName == "" } {
            PrintError "Failed to create a temporary file"
        }
        set TargetFlipThresholdBlurBinaryFileName [CreateFileName "Volume"]
        if { $TargetFlipThresholdBlurBinaryFileName == "" } {
            PrintError "Failed to create a temporary file"
        }

        if { $flip_is_necessary } {
            $LOGIC PrintText "flip..."
            set ret [$CTHandBoneHelper flip $inputFileName $TargetFlipFileName "1" "0" "0"]
        } else {
            # skip flipping
            $LOGIC PrintText "skip flipping..."
            set TargetFlipFileName $inputFileName
        }

        $LOGIC PrintText "threshold..."
        #TODO
        # set fiducialfile "/tmp/Subject2.fcsv"
        #TODO
        # set logfile "/tmp/logfile.txt"
        #set ret [$CTHandBoneHelper fiducial_threshold $TargetFlipFileName $TargetFlipThresholdFileName $fiducialfile $logfile]
        set ret [$CTHandBoneHelper fiducial_threshold $TargetFlipFileName $TargetFlipThresholdFileName]

        $LOGIC PrintText "blur..."
        set ret [$CTHandBoneHelper blur $TargetFlipThresholdFileName $TargetFlipThresholdBlurFileName "1.5" "5"]

        $LOGIC PrintText "binary..."
        set ret [$CTHandBoneHelper binary_threshold $TargetFlipThresholdBlurFileName $TargetFlipThresholdBlurBinaryFileName "0" "30"]

        $LOGIC PrintText "largest..."
        set ret [$CTHandBoneHelper largest_component $TargetFlipThresholdBlurBinaryFileName $outputFileName]

        $LOGIC PrintText "atlas template..."

        $CTHandBoneHelper Delete

    }
}


namespace eval EMSegmenterSimpleTcl {
    # 0 = Do not create a check list for the simple user interface
    # simply remove
    # 1 = Create one - then also define ShowCheckList and
    #     ValidateCheckList where results of checklist are transfered to Preprocessing

    proc CreateCheckList { } {
        return 1
    }

    proc ShowCheckList { } {
        variable inputChannelGUI
        # Always has to be done initially so that variables are correctly defined
        if { [InitVariables] } {
            PrintError "ShowCheckList: Not all variables are correctly defined!"
            return 1
        }

        $inputChannelGUI DefineTextLabel "Is the subject right handed?" 0
        $inputChannelGUI DefineCheckButton "- Are you providing a right hand scan?" 0 $EMSegmenterPreProcessingTcl::rightHandFlagID


        # Define this at the end of the function so that values are set by corresponding MRML node
        $inputChannelGUI SetButtonsFromMRML
        return 0

    }

    proc ValidateCheckList { } {
        return 0
    }

    proc PrintError { TEXT } {
        puts stderr "TCLMRI: ERROR:EMSegmenterSimpleTcl::${TEXT}"
    }
}
