#!/usr/bin/env python3
"""
AWS IoT Fleet Provisioning Setup Script for SunGridNova Inverters
Creates:
1. Provisioning IAM Role & Trust Policy
2. Provisioning Template ('SunGridNova_Provisioning_Template')
3. Claim Policy ('SunGridNova_Claim_Policy') for the Claim Certificate
4. Inverter Operating Policy ('SunGridNova_Inverter_Policy')
"""

import json
import argparse
import sys

TEMPLATE_BODY = {
    "Parameters": {
        "SerialNumber": {
            "Type": "String"
        },
        "MAC": {
            "Type": "String"
        },
        "VendorGroup": {
            "Type": "String"
        }
    },
    "Resources": {
        "thing": {
            "Type": "AWS::IoT::Thing",
            "Properties": {
                "ThingName": {
                    "Fn::Join": ["", ["SunGridNova-", {"Ref": "SerialNumber"}]]
                },
                "AttributePayload": {
                    "version": "1.0",
                    "MAC": {"Ref": "MAC"},
                    "Vendor": {"Ref": "VendorGroup"}
                },
                "ThingGroups": [
                    {"Ref": "VendorGroup"}
                ]
            }
        },
        "certificate": {
            "Type": "AWS::IoT::Certificate",
            "Properties": {
                "CertificateId": {"Ref": "AWS::IoT::Certificate::Id"},
                "Status": "Active"
            }
        },
        "policy": {
            "Type": "AWS::IoT::Policy",
            "Properties": {
                "PolicyName": "SunGridNova_Inverter_Policy"
            }
        }
    }
}

CLAIM_POLICY_DOCUMENT = {
    "Version": "2012-10-17",
    "Statement": [
        {
            "Effect": "Allow",
            "Action": [
                "iot:Connect"
            ],
            "Resource": "*"
        },
        {
            "Effect": "Allow",
            "Action": [
                "iot:Publish",
                "iot:Receive"
            ],
            "Resource": [
                "arn:aws:iot:*:*:topic/$aws/certificates/create/*",
                "arn:aws:iot:*:*:topic/$aws/provisioning-templates/SunGridNova_Provisioning_Template/provision/*"
            ]
        },
        {
            "Effect": "Allow",
            "Action": [
                "iot:Subscribe"
            ],
            "Resource": [
                "arn:aws:iot:*:*:topicfilter/$aws/certificates/create/*",
                "arn:aws:iot:*:*:topicfilter/$aws/provisioning-templates/SunGridNova_Provisioning_Template/provision/*"
            ]
        }
    ]
}

INVERTER_POLICY_DOCUMENT = {
    "Version": "2012-10-17",
    "Statement": [
        {
            "Effect": "Allow",
            "Action": [
                "iot:Connect",
                "iot:Publish",
                "iot:Subscribe",
                "iot:Receive"
            ],
            "Resource": "*"
        }
    ]
}

ROLE_TRUST_POLICY = {
    "Version": "2012-10-17",
    "Statement": [
        {
            "Effect": "Allow",
            "Principal": {
                "Service": "iot.amazonaws.com"
            },
            "Action": "sts:AssumeRole"
        }
    ]
}

def generate_cli_setup(region="ap-southeast-2", account_id="<YOUR_ACCOUNT_ID>"):
    template_json_escaped = json.dumps(TEMPLATE_BODY).replace('"', '\\"')
    claim_policy_escaped = json.dumps(CLAIM_POLICY_DOCUMENT).replace('"', '\\"')
    inverter_policy_escaped = json.dumps(INVERTER_POLICY_DOCUMENT).replace('"', '\\"')
    trust_policy_escaped = json.dumps(ROLE_TRUST_POLICY).replace('"', '\\"')

    script = f"""
# ==============================================================================
# AWS IoT Core Fleet Provisioning Setup Script (Region: {region})
# ==============================================================================

# 1. Create Inverter Operating Policy
aws iot create-policy \\
  --policy-name "SunGridNova_Inverter_Policy" \\
  --policy-document "{inverter_policy_escaped}" \\
  --region {region}

# 2. Create Claim Policy (Permissions for the factory Claim Certificate)
aws iot create-policy \\
  --policy-name "SunGridNova_Claim_Policy" \\
  --policy-document "{claim_policy_escaped}" \\
  --region {region}

# 3. Create IAM Role for IoT Provisioning Template
aws iam create-role \\
  --role-name "SunGridNova_IoT_Provisioning_Role" \\
  --assume-role-policy-document "{trust_policy_escaped}"

aws iam attach-role-policy \\
  --role-name "SunGridNova_IoT_Provisioning_Role" \\
  --policy-arn "arn:aws:iam::aws:policy/service-role/AWSIoTThingsRegistration"

# 4. Create Provisioning Template
aws iot create-provisioning-template \\
  --template-name "SunGridNova_Provisioning_Template" \\
  --description "Auto-registers SunGridNova inverters under Vendor Thing Groups" \\
  --template-body "{template_json_escaped}" \\
  --provisioning-role-arn "arn:aws:iam::{account_id}:role/SunGridNova_IoT_Provisioning_Role" \\
  --enabled \\
  --region {region}

# 5. Attach Claim Policy to your factory Claim Certificate (Replace <CLAIM_CERT_ARN>)
# aws iot attach-policy --policy-name "SunGridNova_Claim_Policy" --target "<CLAIM_CERT_ARN>" --region {region}
"""
    return script

def main():
    parser = argparse.ArgumentParser(description="Generate AWS Fleet Provisioning Setup Commands")
    parser.add_argument("--region", default="ap-southeast-2", help="AWS Region")
    parser.add_argument("--account-id", default="<YOUR_ACCOUNT_ID>", help="AWS Account ID")
    args = parser.parse_args()

    print(generate_cli_setup(args.region, args.account_id))

if __name__ == "__main__":
    main()
